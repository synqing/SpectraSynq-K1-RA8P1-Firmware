static void precompute_goertzel_constants() {
  for (uint16_t i = 0; i < NUM_FREQS; i++) {
    frequencies[i].target_freq = notes[i + CONFIG.NOTE_OFFSET];
    float neighbor_left, neighbor_right;
    if (i == 0) { neighbor_left = notes[i + CONFIG.NOTE_OFFSET]; neighbor_right = notes[i + CONFIG.NOTE_OFFSET + 1]; }
    else if (i == NUM_FREQS - 1) { neighbor_left = notes[i + CONFIG.NOTE_OFFSET - 1]; neighbor_right = notes[i + CONFIG.NOTE_OFFSET]; }
    else { neighbor_left = notes[i + CONFIG.NOTE_OFFSET - 1]; neighbor_right = notes[i + CONFIG.NOTE_OFFSET + 1]; }
    float max_distance_hz = std::max(std::fabs(neighbor_left - frequencies[i].target_freq), std::fabs(neighbor_right - frequencies[i].target_freq));
    frequencies[i].block_size = uint16_t(CONFIG.SAMPLE_RATE / (max_distance_hz * 2.0f));
    if (frequencies[i].block_size > 2000) frequencies[i].block_size = 2000;
    frequencies[i].inv_block_size_half = frequencies[i].block_size ? 2.0f / frequencies[i].block_size : 0;
    float k = int(0.5f + ((frequencies[i].block_size * frequencies[i].target_freq) / CONFIG.SAMPLE_RATE));
    float w = (2.0f * float(M_PI) * k) / frequencies[i].block_size;
    frequencies[i].coeff_q14 = int32_t((1 << 14) * (2.0f * std::cos(w)));
    float freq = frequencies[i].target_freq;
    float tilt = freq < 200.f ? 1.30f : freq > 3000.f ? 0.85f : 1.0f;
    spectral_tilt_lut[i] = SQ15x16(tilt);
  }
}

static int16_t sens_sample(int16_t in) {
  // Full-scale music PCM is not the SPH0645 integer domain. Sensitivity 2.4 is
  // kept, and full scale lands at 2400, above the stored sweet-spot floor of 302
  // and inside the range the Goertzel shift (>> 6) was written for.
  int32_t x = int32_t(in) * 2400 / 32767;
  if (x > 32767) x = 32767;
  if (x < -32767) x = -32767;
  return int16_t(x);
}

static void condition_window(const int16_t* pcm, int pcm_n, int end) {
  for (int i = 0; i < SAMPLE_HISTORY_LENGTH; i++) {
    int src = end - SAMPLE_HISTORY_LENGTH + i;
    sample_window[i] = (src >= 0 && src < pcm_n) ? sens_sample(pcm[src]) : 0;
  }
  max_waveform_val_raw = 0;
  for (int i = SAMPLE_HISTORY_LENGTH - CONFIG.SAMPLES_PER_CHUNK; i < SAMPLE_HISTORY_LENGTH; i++) {
    uint32_t a = uint32_t(std::abs(int(sample_window[i])));
    if (a > max_waveform_val_raw) max_waveform_val_raw = a;
  }
  max_waveform_val = float(max_waveform_val_raw) - CONFIG.SWEET_SPOT_MIN_LEVEL;
  if (max_waveform_val > max_waveform_val_follower) {
    float delta = max_waveform_val - max_waveform_val_follower;
    max_waveform_val_follower += delta * 0.25f;
  } else if (max_waveform_val < max_waveform_val_follower) {
    float delta = max_waveform_val_follower - max_waveform_val;
    max_waveform_val_follower -= delta * 0.005f;
    if (max_waveform_val_follower < CONFIG.SWEET_SPOT_MIN_LEVEL) max_waveform_val_follower = CONFIG.SWEET_SPOT_MIN_LEVEL;
  }
  float waveform_peak_scaled_raw = max_waveform_val / max_waveform_val_follower;
  if (waveform_peak_scaled_raw > waveform_peak_scaled) waveform_peak_scaled += (waveform_peak_scaled_raw - waveform_peak_scaled) * 0.25f;
  else waveform_peak_scaled -= (waveform_peak_scaled - waveform_peak_scaled_raw) * 0.25f;
}

static void write_edge(FILE* f, const CRGB16* edge, float gain) {
  for (int i = 0; i < NATIVE_RESOLUTION; i++) {
    for (double c : {double(edge[i].r), double(edge[i].g), double(edge[i].b)}) {
      if (c < 0) c = 0;
      if (c > 1) c = 1;
      uint16_t q = uint16_t(c * gain * 65535.0 + 0.5);
      std::fwrite(&q, 2, 1, f);
    }
  }
}

static void prove_history() {
  Conf saved = CONFIG;
  std::memset(chromagram_smooth, 0, sizeof chromagram_smooth);
  chromagram_smooth[0] = SQ15x16(1);
  std::memset(leds_16_prev, 0, sizeof leds_16_prev);
  for (int i = 0; i < 40; i++) light_mode_bloom(leds_16_prev);
  double carried = 0;
  for (int i = 96; i < 112; i++) carried += double(leds_16[i].r) + double(leds_16[i].g) + double(leds_16[i].b);
  std::memset(leds_16_prev, 0, sizeof leds_16_prev);
  light_mode_bloom(leds_16_prev);
  double once = 0;
  for (int i = 96; i < 112; i++) once += double(leds_16[i].r) + double(leds_16[i].g) + double(leds_16[i].b);
  CONFIG = saved;
  if (!(carried > 0.05 && once < 0.01)) {
    std::fprintf(stderr, "history check failed carried %.4f once %.4f\n", carried, once);
    std::exit(1);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) { std::fprintf(stderr, "usage: may22_donor pcm12800 prefix\n"); return 2; }
  precompute_goertzel_constants();
  prove_history();
  std::memset(leds_16_prev, 0, sizeof leds_16_prev);
  std::memset(leds_16_prev_secondary, 0, sizeof leds_16_prev_secondary);
  std::ifstream in(argv[1], std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), {});
  if (bytes.size() < 4 || bytes.size() % 2) { std::fprintf(stderr, "pcm\n"); return 2; }
  std::vector<int16_t> pcm(bytes.size() / 2);
  std::memcpy(pcm.data(), bytes.data(), bytes.size());
  const double sys_fps = 48.26, led_fps = 115.86;
  const double seconds = double(pcm.size()) / 12800.0;
  const int frames = int(seconds * led_fps);
  FILE* donor = std::fopen((std::string(argv[2]) + ".rgb16le").c_str(), "wb");
  FILE* titan = std::fopen((std::string(argv[2]) + ".titan.rgb16le").c_str(), "wb");
  if (!donor || !titan) return 2;
  CRGB16 wave_last{};
  float wave_peak_last = 0;
  int sys_i = 0;
  Conf primary = CONFIG;
  for (int frame = 0; frame < frames; frame++) {
    double t = frame / led_fps;
    int need = int(t * sys_fps);
    while (sys_i <= need) {
      int end = int((sys_i / sys_fps) * 12800.0);
      if (end > int(pcm.size())) end = int(pcm.size());
      condition_window(pcm.data(), int(pcm.size()), end);
      CONFIG = primary;
      CONFIG.LIGHTSHOW_MODE = LIGHT_MODE_BLOOM;
      process_GDFT();
      calculate_novelty(uint32_t(t * 1000.0));
      if (primary.AUTO_COLOR_SHIFT) process_color_shift();
      sys_i++;
    }
    CONFIG = primary;
    vp_render_secondary_channel = false;
    if (frame == 2000) {
      std::fprintf(stderr, "t %.2f chroma_final_max %.4f waveform_peak %.3f raw %u spec40 %.4f\n",
        t, float(vp_dbg_chroma_final_max), waveform_peak_scaled, max_waveform_val_raw, float(spectrogram[40]));
    }
    get_smooth_spectrogram();
    make_smooth_chromagram();
    light_mode_bloom(leds_16_prev);
    render_bulb_cover();
    apply_brightness();
    apply_incandescent_filter();
    clip_led_values(leds_16);
    CRGB16 top[NATIVE_RESOLUTION];
    std::memcpy(top, leds_16, sizeof top);
    CONFIG.LIGHTSHOW_MODE = SECONDARY_LIGHTSHOW_MODE;
    CONFIG.PHOTONS = SECONDARY_PHOTONS;
    CONFIG.CHROMA = SECONDARY_CHROMA;
    CONFIG.MOOD = SECONDARY_MOOD;
    CONFIG.MIRROR_ENABLED = SECONDARY_MIRROR_ENABLED;
    CONFIG.SATURATION = SECONDARY_SATURATION;
    CONFIG.AUTO_COLOR_SHIFT = SECONDARY_AUTO_COLOR_SHIFT;
    vp_render_secondary_channel = true;
    std::memcpy(leds_16, leds_16_prev_secondary, sizeof leds_16);
    light_mode_waveform_fast(leds_16_prev_secondary, wave_last, wave_peak_last);
    std::memcpy(leds_16_prev_secondary, leds_16, sizeof leds_16);
    float secondary_gain = SECONDARY_PHOTONS * SECONDARY_PHOTONS * silent_scale;
    for (int i = 0; i < NATIVE_RESOLUTION; i++) {
      leds_16[i].r = leds_16[i].r * secondary_gain;
      leds_16[i].g = leds_16[i].g * secondary_gain;
      leds_16[i].b = leds_16[i].b * secondary_gain;
    }
    clip_led_values(leds_16);
    write_edge(donor, top, 1.f);
    write_edge(donor, leds_16, 1.f);
    write_edge(titan, top, 24.f / 255.f);
    write_edge(titan, leds_16, 24.f / 255.f);
    CONFIG = primary;
    vp_render_secondary_channel = false;
  }
  std::fclose(donor);
  std::fclose(titan);
  std::printf("frames %d led_fps %.2f system_fps %.2f seconds %.2f\n", frames, led_fps, sys_fps, seconds);
  return 0;
}
