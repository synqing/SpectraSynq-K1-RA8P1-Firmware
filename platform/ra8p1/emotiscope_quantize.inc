// Exact Emotiscope.HIL v1.1_build/led_driver.h functions, source lines 51-57 and 205-236.
void init_random_dither_error(){
	for(uint16_t i = 0; i < NUM_LEDS; i++){
		dither_error[ i ].r = random( 0, 255 ) / 255.0;
		dither_error[ i ].g = random( 0, 255 ) / 255.0;
		dither_error[ i ].b = random( 0, 255 ) / 255.0;
	}
}

void quantize_color_error(bool temporal_dithering){
	memcpy(leds_scaled, leds, NUM_LEDS * sizeof(CRGBF));
	dsps_mulc_f32_ansi((float*)leds, (float*)leds_scaled, NUM_LEDS*3, 255.0, 1, 1);

	if(temporal_dithering == true){
		for (uint16_t i = 0; i < NUM_LEDS; i++) {
			raw_led_data[3*i+1] = (uint8_t)(leds_scaled[i].r);
			raw_led_data[3*i+0] = (uint8_t)(leds_scaled[i].g);
			raw_led_data[3*i+2] = (uint8_t)(leds_scaled[i].b);

			float new_error_r = leds_scaled[i].r - raw_led_data[3*i+1];
			float new_error_g = leds_scaled[i].g - raw_led_data[3*i+0];
			float new_error_b = leds_scaled[i].b - raw_led_data[3*i+2];

			const float dither_error_threshold = 0.055;
			if(new_error_r >= dither_error_threshold){ dither_error[i].r += new_error_r; }
			if(new_error_g >= dither_error_threshold){ dither_error[i].g += new_error_g; }
			if(new_error_b >= dither_error_threshold){ dither_error[i].b += new_error_b; }

			if(dither_error[i].r >= 1.0){ raw_led_data[3*i+1] += 1; dither_error[i].r -= 1.0; }
			if(dither_error[i].g >= 1.0){ raw_led_data[3*i+0] += 1; dither_error[i].g -= 1.0; }
			if(dither_error[i].b >= 1.0){ raw_led_data[3*i+2] += 1; dither_error[i].b -= 1.0; }
		}
	}
	else{
		for (uint16_t i = 0; i < NUM_LEDS; i++) {
			raw_led_data[3*i+1] = round(leds_scaled[i].r);
			raw_led_data[3*i+0] = round(leds_scaled[i].g);
			raw_led_data[3*i+2] = round(leds_scaled[i].b);
		}
	}
}
