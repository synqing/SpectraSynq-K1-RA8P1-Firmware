void draw_neutral() {
	if(configuration.mirror_mode == true){ // Mirror mode
		for (uint16_t i = 0; i < (NUM_LEDS >> 1); i++) {
			float progress = num_leds_float_lookup[i<<1];
			CRGBF color = hsv_at(
				get_color_range_hue(progress), 
				configuration.saturation,
				1.0, (NUM_LEDS>>1)+i
);

			leds[ (NUM_LEDS>>1)    + i] = color;
			leds[((NUM_LEDS>>1)-1) - i] = color;
		}
	}
	else{ // Non mirror
		for (uint16_t i = 0; i < NUM_LEDS; i++) {
			float progress = num_leds_float_lookup[i];
			CRGBF color = hsv_at(
				get_color_range_hue(progress), 
				configuration.saturation,
				1.0, i
);
			
			leds[i] = color;
		}
	}
}