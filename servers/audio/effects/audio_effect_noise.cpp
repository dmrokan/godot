/**************************************************************************/
/*  audio_effect_noise.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "audio_effect_noise.h"

#include "servers/audio_server.h"

void AudioEffectNoiseFrame::update_parameters() {
	if (effect == nullptr)
		return;

	rng_params.std = effect->std;
}

AudioFrame AudioEffectNoiseFrame::next() {
	return AudioFrame();
}

AudioEffectNoiseFrame::AudioEffectNoiseFrame() {
	effect = nullptr;
	rng.instantiate();
	rng_params.std = 0.25;
}

AudioFrame AudioEffectWhiteNoiseFrame::next() {
	float output = rng->randfn(0, rng_params.std);

	return AudioFrame(output, output);
}

AudioFrame AudioEffectBrownNoiseFrame::next() {
	float high_freq_component_weight = 0.2f;
	float output = 2.5f * state.x[0];
	const float integrator_damping = 0.01f;

	state.x[0] = (1.0f - integrator_damping) * state.x[0] + high_freq_component_weight * rng->randfn(0, rng_params.std);

	return AudioFrame(output, output);
}

AudioEffectBrownNoiseFrame::AudioEffectBrownNoiseFrame() {
	state.x[0] = 0;
}

AudioFrame AudioEffectPinkNoiseFrame::next() {
	float output = 0;

	state.x[0] = rng->randfn(0, rng_params.std);
	for (int i = State::n; i > 0; i--) {
		output += params.h[i - 1] * state.x[i];
		state.x[i] = state.x[i - 1];
	}

	return AudioFrame(output, output);
}

AudioEffectPinkNoiseFrame::AudioEffectPinkNoiseFrame() {
	for (int i = 0; i < State::n + 1; i++) {
		state.x[i] = 0.0f;
	}

	float alpha = 0.5f;
	float h0 = 1.0f;
	for (int i = 1; i < State::n + 1; i++) {
		h0 *= (alpha / 2.0f + (float)i - 1.0f) / (float)i;
		params.h[State::n - i] = h0;
	}
}

AudioFrame AudioEffectVioletNoiseFrame::next() {
	float r = rng->randfn(0, rng_params.std);
	float output = r - state.x[0];
	state.x[0] = r;

	return AudioFrame(output, output);
}

AudioFrame AudioEffectGrayNoiseFrame::next() {
	float output = 0;

	state.x[0] = rng->randfn(0, rng_params.std);
	for (int i = State::n; i > 0; i--) {
		output += params.h[i - 1] * state.x[i];
		state.x[i] = state.x[i - 1];
	}

	return AudioFrame(output, output);
}

AudioEffectGrayNoiseFrame::AudioEffectGrayNoiseFrame() {
	for (int i = 0; i < State::n + 1; i++) {
		state.x[i] = 0.0f;
	}

	params.h[0] = 0.13095192f;
	params.h[1] = 0.14271321f;
	params.h[2] = -0.10107508f;
	params.h[3] = 0.65481989f;
	params.h[4] = -0.10107508f;
	params.h[5] = 0.14271321f;
	params.h[6] = 0.13095192f;
}

/////////////////

void AudioEffectNoiseInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	if (base.is_null() || base->frame.is_null())
		return;

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i] + base->gain_linear * base->next_frame();
	}
}

Ref<AudioEffectInstance> AudioEffectNoise::instantiate() {
	Ref<AudioEffectNoiseInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectNoise>(this);
	return ins;
}

AudioFrame AudioEffectNoise::next_frame() const {
	return frame->next();
}

void AudioEffectNoise::set_type(const String &p_type) {
	if (frame.is_valid())
		frame.unref();

	if (p_type == "Brown") {
		type = p_type;
		Ref<AudioEffectBrownNoiseFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else if (p_type == "Pink") {
		type = p_type;
		Ref<AudioEffectPinkNoiseFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else if (p_type == "Violet") {
		type = p_type;
		Ref<AudioEffectVioletNoiseFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else if (p_type == "Gray") {
		type = p_type;
		Ref<AudioEffectGrayNoiseFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else {
		type = "White";
		Ref<AudioEffectWhiteNoiseFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	}

	if (frame.is_valid()) {
		frame->effect = this;
		frame->update_parameters();
	}
}

String AudioEffectNoise::get_type() const {
	return type;
}

void AudioEffectNoise::set_std(float p_std) {
	std = p_std;
	if (frame.is_valid()) {
		frame->update_parameters();
	}
}

float AudioEffectNoise::get_std() const {
	return std;
}

void AudioEffectNoise::set_gain(float p_gain) {
	gain = p_gain;
	gain_linear = Math::db_to_linear(gain);
	if (frame.is_valid()) {
		frame->update_parameters();
	}
}

float AudioEffectNoise::get_gain() const {
	return gain;
}

void AudioEffectNoise::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_type", "phase"), &AudioEffectNoise::set_type);
	ClassDB::bind_method(D_METHOD("get_type"), &AudioEffectNoise::get_type);
	ClassDB::bind_method(D_METHOD("set_gain", "gain"), &AudioEffectNoise::set_gain);
	ClassDB::bind_method(D_METHOD("get_gain"), &AudioEffectNoise::get_gain);
	ClassDB::bind_method(D_METHOD("set_std", "std"), &AudioEffectNoise::set_std);
	ClassDB::bind_method(D_METHOD("get_std"), &AudioEffectNoise::get_std);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "-80,24,0.01,suffix:dB"), "set_gain", "get_gain");
	ADD_PROPERTY_DEFAULT("gain", 0.0);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type", PROPERTY_HINT_ENUM, "White,Brown,Pink,Violet,Gray"), "set_type", "get_type");
	ADD_PROPERTY_DEFAULT("type", "White");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "deviation", PROPERTY_HINT_RANGE, "0,0.5,0.01"), "set_std", "get_std");
	ADD_PROPERTY_DEFAULT("std", 0.25);
}

AudioEffectNoise::AudioEffectNoise() {
	type = "White";
	std = 0.25;
	gain = 0;
	gain_linear = 1;
	Ref<AudioEffectWhiteNoiseFrame> tmp;
	tmp.instantiate();
	tmp->effect = this;
	tmp->update_parameters();
	frame = tmp;
}
