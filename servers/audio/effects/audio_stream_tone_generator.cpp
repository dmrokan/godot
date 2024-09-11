/**************************************************************************/
/*  audio_stream_tone_generator.cpp                                       */
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

#include "audio_stream_tone_generator.h"

AudioFrame AudioGeneratorFrame::next() {
	return AudioFrame();
}

AudioGeneratorFrame::AudioGeneratorFrame() {
	stream = nullptr;
}

void AudioToneGeneratorFrame::update_parameters() {
	if (stream == nullptr)
		return;

	double normalized_frequency = 2.0 * (double)stream->frequency / stream->mix_rate;
	double normalized_damping = Math::exp(-(double)stream->damping / stream->mix_rate);
	double d = normalized_damping;
	double om = Math_PI * normalized_frequency;
	double c = -d * Math::cos(om);
	// NOTE: (d*cos(om))^2 = c^2 <= d^2 is always satisfied.
	double b = Math::sqrt(d * d - c * c);
	params.A[0] = -2 * c;
	params.A[1] = -d * d;
	params.C = b;

	double psi = Math::deg_to_rad((double)stream->phase);
	double C = params.C;
	double x1_0 = Math::cos(psi) / C;
	double x1_1 = Math::cos(Math_PI * normalized_frequency + psi) / C;
	double x2_0 = (x1_1 - params.A[0] * x1_0) / params.A[1];
	state.x[0] = x1_0;
	state.x[1] = x2_0;
}

AudioFrame AudioToneGeneratorFrame::next() {
	float output = params.C * state.x[0];
	double tmp = params.A[0] * state.x[0] + params.A[1] * state.x[1];
	state.x[1] = state.x[0];
	state.x[0] = tmp;

	return AudioFrame(output, output);
}

void AudioSawGeneratorFrame::update_parameters() {
	if (stream == nullptr)
		return;

	params.inc = 2.0 * (double)stream->frequency / stream->mix_rate;
	params.damping = Math::exp(-(double)stream->damping / stream->mix_rate);

	double psi = Math::deg_to_rad((double)stream->phase);
	state.x[0] = 2.0 * psi / Math_PI - 1.0;
	state.x[1] = 0.0;
	state.x[2] = 1.0;
}

AudioFrame AudioSawGeneratorFrame::next() {
	const double normalized_low_pass_cutoff = 0.99;
	float output = state.x[1] * state.x[2];

	state.x[1] = (1.0 - normalized_low_pass_cutoff) * state.x[1] + normalized_low_pass_cutoff * state.x[0];
	state.x[2] *= params.damping;

	state.x[0] += params.inc;
	if (state.x[0] > 1.0)
		state.x[0] = -1.0;

	return AudioFrame(output, output);
}

AudioFrame AudioRectGeneratorFrame::next() {
	const double normalized_low_pass_cutoff = 0.99;
	float output = state.x[1] * state.x[2];

	state.x[1] = (1.0 - normalized_low_pass_cutoff) * state.x[1] + normalized_low_pass_cutoff * SIGN(state.x[0]);
	state.x[2] *= params.damping;

	state.x[0] += params.inc;
	if (state.x[0] > 1.0)
		state.x[0] = -1.0;

	return AudioFrame(output, output);
}

void AudioVanDerPolGeneratorFrame::update_parameters() {
	if (stream == nullptr)
		return;

	AudioGeneratorFrame::update_parameters();

	double eps = 1.9;
	double alpha = eps / 2;
	double beta = Math::sqrt(1 - alpha * alpha);
	double f = CLAMP(stream->frequency, 100, stream->mix_rate / 2.0);
	params.T = 1.0 / (stream->mix_rate / f);
	double c1 = Math::exp(alpha * params.T);
	double c2 = beta * params.T;
	params.psi = (c1 * (Math::cos(c2) - eps * Math::sin(c2) / 2.0 / beta) - 1.0) / params.T;
	params.phi = c1 * Math::sin(c2) / beta / params.T;
	params.damping = Math::exp(-(double)stream->damping / stream->mix_rate);

	state.x[0] = 1.0;
	state.x[1] = 0.0;
	state.x[2] = 1.0;
}

AudioFrame AudioVanDerPolGeneratorFrame::next() {
	double eps = 1.9;
	double p = state.x[0];
	double q = state.x[1];
	float output = q * state.x[2];

	double dp = params.psi * p + params.phi * q;
	p += params.T * dp;
	double dq = params.psi * q + params.phi * (-state.x[0] + eps * (1.0 - p * p) * q);
	q += params.T * dq;

	state.x[0] = p;
	state.x[1] = q;
	state.x[2] *= params.damping;

	return AudioFrame(output, output);
}

////////////////

AudioFrame AudioStreamToneGenerator::next_frame() const {
	return frame->next();
}

void AudioStreamToneGenerator::set_mix_rate(float p_mix_rate) {
	mix_rate = p_mix_rate;
	frame->update_parameters();
}

float AudioStreamToneGenerator::get_mix_rate() const {
	return mix_rate;
}

void AudioStreamToneGenerator::set_buffer_length(float p_seconds) {
	buffer_len = p_seconds;
}

float AudioStreamToneGenerator::get_buffer_length() const {
	return buffer_len;
}

void AudioStreamToneGenerator::set_type(const String &p_type) {
	if (frame.is_valid())
		frame.unref();

	if (p_type == "Tone") {
		type = p_type;
		Ref<AudioToneGeneratorFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else if (p_type == "Saw") {
		type = p_type;
		Ref<AudioSawGeneratorFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else if (p_type == "Rect") {
		type = p_type;
		Ref<AudioRectGeneratorFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	} else if (p_type == "VanDerPol") {
		type = p_type;
		Ref<AudioVanDerPolGeneratorFrame> tmp;
		tmp.instantiate();
		frame = tmp;
	}

	if (frame.is_valid()) {
		frame->stream = this;
		frame->update_parameters();
	}
}

String AudioStreamToneGenerator::get_type() const {
	return type;
}

void AudioStreamToneGenerator::set_frequency(float p_frequency) {
	frequency = MIN(p_frequency, mix_rate / 2.0);
	if (frame.is_valid())
		frame->update_parameters();
}

float AudioStreamToneGenerator::get_frequency() const {
	return frequency;
}

void AudioStreamToneGenerator::set_damping(float p_damping) {
	damping = p_damping;
	if (frame.is_valid())
		frame->update_parameters();
}

float AudioStreamToneGenerator::get_damping() const {
	return damping;
}

void AudioStreamToneGenerator::set_phase(float p_phase) {
	phase = p_phase;
	if (frame.is_valid())
		frame->update_parameters();
}

float AudioStreamToneGenerator::get_phase() const {
	return phase;
}

void AudioStreamToneGenerator::set_gain(float p_gain) {
	gain = p_gain;
	gain_linear = Math::db_to_linear(gain);
}

float AudioStreamToneGenerator::get_gain() const {
	return gain;
}

Ref<AudioStreamPlayback> AudioStreamToneGenerator::instantiate_playback() {
	Ref<AudioStreamToneGeneratorPlayback> playback;
	playback.instantiate();
	playback->generator = this;
	return playback;
}

String AudioStreamToneGenerator::get_stream_name() const {
	return "UserFeed";
}

double AudioStreamToneGenerator::get_length() const {
	return buffer_len;
}

bool AudioStreamToneGenerator::is_monophonic() const {
	return true;
}

void AudioStreamToneGenerator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mix_rate", "hz"), &AudioStreamToneGenerator::set_mix_rate);
	ClassDB::bind_method(D_METHOD("get_mix_rate"), &AudioStreamToneGenerator::get_mix_rate);

	ClassDB::bind_method(D_METHOD("set_buffer_length", "seconds"), &AudioStreamToneGenerator::set_buffer_length);
	ClassDB::bind_method(D_METHOD("get_buffer_length"), &AudioStreamToneGenerator::get_buffer_length);

	ClassDB::bind_method(D_METHOD("set_type", "frequency"), &AudioStreamToneGenerator::set_type);
	ClassDB::bind_method(D_METHOD("get_type"), &AudioStreamToneGenerator::get_type);
	ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &AudioStreamToneGenerator::set_frequency);
	ClassDB::bind_method(D_METHOD("get_frequency"), &AudioStreamToneGenerator::get_frequency);
	ClassDB::bind_method(D_METHOD("set_phase", "phase"), &AudioStreamToneGenerator::set_phase);
	ClassDB::bind_method(D_METHOD("get_phase"), &AudioStreamToneGenerator::get_phase);
	ClassDB::bind_method(D_METHOD("set_damping", "damping"), &AudioStreamToneGenerator::set_damping);
	ClassDB::bind_method(D_METHOD("get_damping"), &AudioStreamToneGenerator::get_damping);
	ClassDB::bind_method(D_METHOD("set_gain", "gain"), &AudioStreamToneGenerator::set_gain);
	ClassDB::bind_method(D_METHOD("get_gain"), &AudioStreamToneGenerator::get_gain);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "type", PROPERTY_HINT_ENUM, "Tone,Saw,Rect,VanDerPol"), "set_type", "get_type");
	ADD_PROPERTY_DEFAULT("type", "Tone");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mix_rate", PROPERTY_HINT_RANGE, "20,192000,1,suffix:Hz"), "set_mix_rate", "get_mix_rate");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "buffer_length", PROPERTY_HINT_RANGE, "0.01,10,0.01,suffix:s"), "set_buffer_length", "get_buffer_length");
	ADD_PROPERTY_DEFAULT("buffer_length", 0.5);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency", PROPERTY_HINT_RANGE, "0,48e3,1,suffix:Hz"), "set_frequency", "get_frequency");
	ADD_PROPERTY_DEFAULT("frequency", 400);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping"), "set_damping", "get_damping");
	ADD_PROPERTY_DEFAULT("damping", 0);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "phase", PROPERTY_HINT_RANGE, "0,180,0.1,suffix:deg"), "set_phase", "get_phase");
	ADD_PROPERTY_DEFAULT("phase", 0);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "-80,0,0.01,suffix:dB"), "set_gain", "get_gain");
	ADD_PROPERTY_DEFAULT("gain_db", 0.0f);
}

AudioStreamToneGenerator::AudioStreamToneGenerator() {
	mix_rate = 44100;
	buffer_len = 0.5;
	type = "Tone";
	frequency = 400;
	phase = 0;
	damping = 0;
	gain = 0;
	gain_linear = 1;
	Ref<AudioToneGeneratorFrame> tmp;
	tmp.instantiate();
	frame = tmp;
	frame->stream = this;
	frame->update_parameters();
}

////////////////

int AudioStreamToneGeneratorPlayback::_mix_internal(AudioFrame *p_buffer, int p_frames) {
	if (!active) {
		return 0;
	}

	for (int i = 0; i < p_frames; i++) {
		p_buffer[i] = generator->gain_linear * generator->next_frame();
	}

	mixed += p_frames / generator->get_mix_rate();
	return p_frames;
}

float AudioStreamToneGeneratorPlayback::get_stream_sampling_rate() {
	if (generator != nullptr)
		return generator->get_mix_rate();

	return AudioServer::get_singleton()->get_mix_rate();
}

void AudioStreamToneGeneratorPlayback::start(double p_from_pos) {
	if (mixed == 0.0) {
		begin_resample();
	}
	active = true;
	mixed = 0.0;

	if (generator != nullptr && generator->frame != nullptr)
		generator->frame->update_parameters();
}

void AudioStreamToneGeneratorPlayback::stop() {
	active = false;
}

bool AudioStreamToneGeneratorPlayback::is_playing() const {
	return active;
}

int AudioStreamToneGeneratorPlayback::get_loop_count() const {
	return 0;
}

double AudioStreamToneGeneratorPlayback::get_playback_position() const {
	return mixed;
}

void AudioStreamToneGeneratorPlayback::seek(double p_time) {
	//no seek possible
}

void AudioStreamToneGeneratorPlayback::tag_used_streams() {
	generator->tag_used(0);
}

void AudioStreamToneGeneratorPlayback::_bind_methods() {
}

AudioStreamToneGeneratorPlayback::AudioStreamToneGeneratorPlayback() {
	generator = nullptr;
	active = false;
	mixed = 0;
}
