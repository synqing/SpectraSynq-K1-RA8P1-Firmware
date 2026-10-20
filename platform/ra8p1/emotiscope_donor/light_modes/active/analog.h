float vu_level_smooth = 0.000001;

void draw_analog(){
	profile_function([&]() {
		float mix_speed = 0.005 + 0.145*configuration.speed;

		vu_level_smooth = (vu_level) * mix_speed + vu_level_smooth*(1.0-mix_speed);
		float dot_pos = clip_float(vu_level_smooth);
		const float dot_u=get_color_range_hue(dot_pos);

		if(configuration.mirror_mode == true){
			const float paired_u=get_color_range_hue(dot_pos + 0.5f);
			dot_pos = 0.05 + dot_pos * 0.95;

			draw_palette_dot(leds, NUM_RESERVED_DOTS+0, dot_u, 1.0f, 0.5 + (dot_pos* 0.5), 1.0);
			// The paired dot uses another position in the same permanent palette;
			// retain the original music position, opacity and number of dots.
			draw_palette_dot(leds, NUM_RESERVED_DOTS+1, paired_u, 1.0f, 0.5 + (dot_pos*-0.5), 1.0);
		}
		else{
			draw_palette_dot(leds, NUM_RESERVED_DOTS+0, dot_u, 1.0f, dot_pos, 1.0);
		}
	}, __func__);
}
