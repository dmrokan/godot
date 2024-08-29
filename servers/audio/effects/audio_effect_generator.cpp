/**************************************************************************/
/*  audio_effect_generator.cpp                                            */
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

#include "audio_effect_generator.h"

#include "servers/audio_server.h"

#define GET_SYSTEM_SAMPLE_RATE(...) (AudioServer::get_singleton()->get_mix_rate())

float AudioEffectBaseGeneratorInstance::get_output() {
	return 0.0f;
}

void AudioEffectBaseGeneratorInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i] + gain * get_output() + offset;
	}
}

float AudioEffectToneGeneratorInstance::get_output() {
	float output = params.C * state.x[0];
	double tmp = params.A[0] * state.x[0] + params.A[1] * state.x[1];
	state.x[1] = state.x[0];
	state.x[0] = tmp;

	return output;
}

float AudioEffectSawGeneratorInstance::get_output() {
	float output = state.x[0];
	state.x[0] += params.inc;
	if (state.x[0] > 1.0f)
		state.x[0] = -1.0f;

	return output;
}

float AudioEffectWhiteNoiseGeneratorInstance::get_output() {
	return rng->randfn(params.mean, params.std);
}

AudioEffectWhiteNoiseGeneratorInstance::AudioEffectWhiteNoiseGeneratorInstance() {
	rng.instantiate();
}

float AudioEffectBrownNoiseGeneratorInstance::get_output() {
	float output = state.x[0];
	float high_freq_component_weight = 0.1f;
	const float integrator_damping = 0.01f;

	state.x[0] = (1.0f - integrator_damping) * state.x[0] + high_freq_component_weight * rng->randfn(params.mean, params.std);

	return output;
}

float AudioEffectPinkNoiseGeneratorInstance::get_output() {
	float output = state.x[0];
	float high_freq_component_weight = 0.2f;

	state.x[0] = high_freq_component_weight * rng->randfn(params.mean, params.std);
	for (int i = State::n; i > 0; i--) {
		state.x[0] += filter.h[i - 1] * state.x[i];
		state.x[i] = state.x[i - 1];
	}

	return output;
}

void AudioEffectPinkNoiseGeneratorInstance::reset_state() {
	for (int i = 0; i < State::n + 1; i++) {
		state.x[i] = 0.0f;
	}
}

AudioEffectPinkNoiseGeneratorInstance::Filter::Filter() {
	// NOTE: Found by trial-and-error
	float alpha = 0.4f;
	float h0 = 1.0f;
	for (int i = 1; i < State::n + 1; i++) {
		h0 *= (alpha / 2.0f + (float)i - 1.0f) / (float)i;
		h[State::n - i] = h0;
	}
}

void AudioEffectGeneratorInstance::set_generator_type(GeneratorType type) {
	switch (generator_type) {
		case TYPE_TONE:
			generator_instance.tone.unref();
			break;
		case TYPE_SAW:
			generator_instance.saw.unref();
			break;
		case TYPE_WHITE_NOISE:
			generator_instance.white_noise.unref();
			break;
		case TYPE_BROWN_NOISE:
			generator_instance.white_noise.unref();
			break;
		case TYPE_PINK_NOISE:
			generator_instance.pink_noise.unref();
			break;
		default:
			break;
	}

	generator_type = type;
}

void AudioEffectGeneratorInstance::update_parameters() {
	switch (generator_type) {
		case TYPE_TONE: {
			double normalized_frequency = 2.0 * base->params.frequency / (double)base->params.sampling_frequency;
			double normalized_damping = exp(-base->params.damping / (double)base->params.sampling_frequency);
			double d = normalized_damping;
			double om = Math_PI * normalized_frequency;
			double c = -d * Math::cos(om);
			// NOTE: c^2 = (d*cos(om))^2 <= d^2 is always satisfied.
			double b = Math::sqrt(d * d - c * c);

			Ref<AudioEffectToneGeneratorInstance> tone = get_generator_insance<AudioEffectToneGeneratorInstance>();
			tone->params.A[0] = -2 * c;
			tone->params.A[1] = -d * d;
			tone->params.C = b;
			tone->gain = base->params.gain_linear;
			tone->offset = base->params.offset;
			break;
		}
		case TYPE_SAW: {
			double inc = 2.0 * (double)base->params.frequency / base->params.sampling_frequency;
			Ref<AudioEffectSawGeneratorInstance> saw = get_generator_insance<AudioEffectSawGeneratorInstance>();
			saw->params.inc = inc;
			saw->gain = base->params.gain_linear;
			saw->offset = base->params.offset;
			break;
		}
		case TYPE_WHITE_NOISE: {
			Ref<AudioEffectWhiteNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectWhiteNoiseGeneratorInstance>();
			noise->gain = base->params.gain_linear;
			noise->offset = base->params.offset;
			noise->params.mean = base->params.mean;
			noise->params.std = base->params.std;
			break;
		}
		case TYPE_BROWN_NOISE: {
			Ref<AudioEffectBrownNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectBrownNoiseGeneratorInstance>();
			noise->gain = base->params.gain_linear;
			noise->offset = base->params.offset;
			noise->params.mean = base->params.mean;
			noise->params.std = base->params.std;
			break;
		}
		case TYPE_PINK_NOISE: {
			Ref<AudioEffectPinkNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectPinkNoiseGeneratorInstance>();
			noise->gain = base->params.gain_linear;
			noise->offset = base->params.offset;
			noise->params.mean = base->params.mean;
			noise->params.std = base->params.std;
			break;
		}
		default:
			break;
	}
}

void AudioEffectGeneratorInstance::update_initial_conditions() {
	switch (generator_type) {
		case TYPE_TONE: {
			double psi = Math::deg_to_rad((double)base->params.phase);
			Ref<AudioEffectToneGeneratorInstance> tone = get_generator_insance<AudioEffectToneGeneratorInstance>();
			double C = tone->params.C;
			double x1_0 = Math::cos(psi) / C;
			double normalized_frequency = 2.0 * base->params.frequency / (double)base->params.sampling_frequency;
			double x1_1 = Math::cos(Math_PI * normalized_frequency + psi) / C;
			double x2_0 = (x1_1 - tone->params.A[0] * x1_0) / tone->params.A[1];
			tone->state.x[0] = x1_0;
			tone->state.x[1] = x2_0;
			break;
		}
		case TYPE_SAW: {
			double psi = Math::deg_to_rad((double)base->params.phase);
			double ic = 2.0 * psi / Math_PI - 1.0;
			Ref<AudioEffectSawGeneratorInstance> saw = get_generator_insance<AudioEffectSawGeneratorInstance>();
			saw->state.x[0] = ic;
			break;
		}
		case TYPE_WHITE_NOISE: {
			break;
		}
		case TYPE_BROWN_NOISE: {
			Ref<AudioEffectBrownNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectBrownNoiseGeneratorInstance>();
			noise->state.x[0] = 0.0f;
			break;
		}
		case TYPE_PINK_NOISE: {
			Ref<AudioEffectPinkNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectPinkNoiseGeneratorInstance>();
			noise->reset_state();
			break;
		}
		default:
			break;
	}
}

void AudioEffectGeneratorInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	if (base->is_type_updated) {
		set_generator_type(base->params.type);
		base->is_type_updated = false;
		base->is_parameters_updated = true;
		base->should_reset_state = true;
	}
	if (base->is_parameters_updated) {
		update_parameters();
		base->is_parameters_updated = false;
	}
	if (base->should_reset_state) {
		update_initial_conditions();
		base->should_reset_state = false;
	}

	switch (generator_type) {
		case TYPE_TONE: {
			Ref<AudioEffectToneGeneratorInstance> tone = get_generator_insance<AudioEffectToneGeneratorInstance>();
			tone->process(p_src_frames, p_dst_frames, p_frame_count);
			break;
		}
		case TYPE_SAW: {
			Ref<AudioEffectSawGeneratorInstance> saw = get_generator_insance<AudioEffectSawGeneratorInstance>();
			saw->process(p_src_frames, p_dst_frames, p_frame_count);
			break;
		}
		case TYPE_WHITE_NOISE: {
			Ref<AudioEffectWhiteNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectWhiteNoiseGeneratorInstance>();
			noise->process(p_src_frames, p_dst_frames, p_frame_count);
			break;
		}
		case TYPE_BROWN_NOISE: {
			Ref<AudioEffectBrownNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectBrownNoiseGeneratorInstance>();
			noise->process(p_src_frames, p_dst_frames, p_frame_count);
			break;
		}
		case TYPE_PINK_NOISE: {
			Ref<AudioEffectPinkNoiseGeneratorInstance> noise = get_generator_insance<AudioEffectPinkNoiseGeneratorInstance>();
			noise->process(p_src_frames, p_dst_frames, p_frame_count);
			break;
		}
		default:
			break;
	}
}

AudioEffectGeneratorInstance::AudioEffectGeneratorInstance() {
	generator_type = TYPE_TONE;
}

Ref<AudioEffectInstance> AudioEffectGenerator::instantiate() {
	Ref<AudioEffectGeneratorInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectGenerator>(this);
	is_parameters_updated = true;
	should_reset_state = true;
	is_type_updated = true;

	return ins;
}

void AudioEffectGenerator::set_sampling_frequency(float p_sampling_frequency) {
}

float AudioEffectGenerator::get_sampling_frequency() {
	params.sampling_frequency = GET_SYSTEM_SAMPLE_RATE();

	return params.sampling_frequency;
}

void AudioEffectGenerator::set_frequency(float p_frequency) {
	params.frequency = MIN(p_frequency, get_sampling_frequency() / 2);
	is_parameters_updated = true;
}

float AudioEffectGenerator::get_frequency() const {
	return params.frequency;
}

void AudioEffectGenerator::set_damping(float p_damping) {
	params.damping = p_damping;
	is_parameters_updated = true;
}

float AudioEffectGenerator::get_damping() const {
	return params.damping;
}

void AudioEffectGenerator::set_phase(float p_phase) {
	params.phase = p_phase;
	is_parameters_updated = true;
	should_reset_state = true;
}

float AudioEffectGenerator::get_phase() const {
	return params.phase;
}

void AudioEffectGenerator::set_gain_db(float p_gain_db) {
	params.gain_db = p_gain_db;
	params.gain_linear = Math::db_to_linear(params.gain_db);
	is_parameters_updated = true;
}

float AudioEffectGenerator::get_gain_db() const {
	return params.gain_db;
}

void AudioEffectGenerator::set_offset(float p_offset) {
	params.offset = p_offset;
	is_parameters_updated = true;
}

float AudioEffectGenerator::get_offset() const {
	return params.offset;
}

void AudioEffectGenerator::set_mean(float p_mean) {
	params.mean = p_mean;
	is_parameters_updated = true;
}

float AudioEffectGenerator::get_mean() const {
	return params.mean;
}

void AudioEffectGenerator::set_std(float p_std) {
	params.std = p_std;
	is_parameters_updated = true;
}

float AudioEffectGenerator::get_std() const {
	return params.std;
}

void AudioEffectGenerator::set_type(const String &p_type) {
	s_type = p_type;

	AudioEffectGeneratorInstance::GeneratorType type = AudioEffectGeneratorInstance::TYPE_TONE;

	if (s_type == "Tone") {
		type = AudioEffectGeneratorInstance::TYPE_TONE;
	} else if (s_type == "Saw") {
		type = AudioEffectGeneratorInstance::TYPE_SAW;
	} else if (s_type == "WhiteNoise") {
		type = AudioEffectGeneratorInstance::TYPE_WHITE_NOISE;
	} else if (s_type == "BrownNoise") {
		type = AudioEffectGeneratorInstance::TYPE_BROWN_NOISE;
	} else if (s_type == "PinkNoise") {
		type = AudioEffectGeneratorInstance::TYPE_PINK_NOISE;
	}

	params.type = type;
	is_type_updated = true;
}

String AudioEffectGenerator::get_type() const {
	return s_type;
}

void AudioEffectGenerator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sampling_frequency", "sampling_frequency"), &AudioEffectGenerator::set_sampling_frequency);
	ClassDB::bind_method(D_METHOD("set_type", "phase"), &AudioEffectGenerator::set_type);
	ClassDB::bind_method(D_METHOD("get_type"), &AudioEffectGenerator::get_type);
	ClassDB::bind_method(D_METHOD("get_sampling_frequency"), &AudioEffectGenerator::get_sampling_frequency);
	ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &AudioEffectGenerator::set_frequency);
	ClassDB::bind_method(D_METHOD("get_frequency"), &AudioEffectGenerator::get_frequency);
	ClassDB::bind_method(D_METHOD("set_damping", "damping"), &AudioEffectGenerator::set_damping);
	ClassDB::bind_method(D_METHOD("get_damping"), &AudioEffectGenerator::get_damping);
	ClassDB::bind_method(D_METHOD("set_phase", "phase"), &AudioEffectGenerator::set_phase);
	ClassDB::bind_method(D_METHOD("get_phase"), &AudioEffectGenerator::get_phase);
	ClassDB::bind_method(D_METHOD("set_offset", "offset"), &AudioEffectGenerator::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &AudioEffectGenerator::get_offset);
	ClassDB::bind_method(D_METHOD("set_gain_db", "gain_db"), &AudioEffectGenerator::set_gain_db);
	ClassDB::bind_method(D_METHOD("get_gain_db"), &AudioEffectGenerator::get_gain_db);
	ClassDB::bind_method(D_METHOD("set_mean", "mean"), &AudioEffectGenerator::set_mean);
	ClassDB::bind_method(D_METHOD("get_mean"), &AudioEffectGenerator::get_mean);
	ClassDB::bind_method(D_METHOD("set_std", "std"), &AudioEffectGenerator::set_std);
	ClassDB::bind_method(D_METHOD("get_std"), &AudioEffectGenerator::get_std);
	ClassDB::bind_method(D_METHOD("reset"), &AudioEffectGenerator::reset);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sampling_frequency", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_DEFAULT), "set_sampling_frequency", "get_sampling_frequency");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency", PROPERTY_HINT_RANGE, "0,48e3,0.001"), "set_frequency", "get_frequency");
	ADD_PROPERTY_DEFAULT("frequency", 400.0);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping"), "set_damping", "get_damping");
	ADD_PROPERTY_DEFAULT("damping", 0.0f);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phase", PROPERTY_HINT_RANGE, "0,180,0.01"), "set_phase", "get_phase");
	ADD_PROPERTY_DEFAULT("phase", 0.0f);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain_db", PROPERTY_HINT_RANGE, "-80,24,0.01,suffix:dB"), "set_gain_db", "get_gain_db");
	ADD_PROPERTY_DEFAULT("gain_db", 0.0f);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type", PROPERTY_HINT_ENUM, "Tone,Saw,WhiteNoise,BrownNoise,PinkNoise"), "set_type", "get_type");
	ADD_PROPERTY_DEFAULT("type", "Tone");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "offset", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_offset", "get_offset");
	ADD_PROPERTY_DEFAULT("offset", 0.0f);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mean", PROPERTY_HINT_RANGE, "-0.5,0.5,0.001"), "set_mean", "get_mean");
	ADD_PROPERTY_DEFAULT("mean", 0.0f);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "deviation", PROPERTY_HINT_RANGE, "0,0.5,0.01"), "set_std", "get_std");
	ADD_PROPERTY_DEFAULT("std", 0.1f);
}

void AudioEffectGenerator::reset() {
	should_reset_state = true;
}

AudioEffectGenerator::AudioEffectGenerator() {
	params.frequency = 400.0f;
	params.damping = 0.0f;
	params.phase = 0.0f;
	params.gain_db = 0.0f;
	params.gain_linear = 1.0f;
	params.offset = 0.0f;
	params.mean = 0.0f;
	params.std = 0.1f;
	params.type = AudioEffectGeneratorInstance::TYPE_TONE;
	s_type = "Tone";
	is_parameters_updated = true;
	should_reset_state = true;
	is_type_updated = true;
}
