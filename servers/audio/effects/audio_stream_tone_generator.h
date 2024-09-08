/**************************************************************************/
/*  audio_stream_tone_generator.h                                         */
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

#ifndef AUDIO_STREAM_TONE_GENERATOR_H
#define AUDIO_STREAM_TONE_GENERATOR_H

#include "servers/audio/audio_stream.h"

class AudioStreamToneGenerator;

class AudioGeneratorFrame : public RefCounted {
	GDCLASS(AudioGeneratorFrame, RefCounted);
	friend class AudioStreamToneGenerator;

protected:
	AudioStreamToneGenerator *stream;

public:
	virtual void update_parameters() {}
	virtual AudioFrame next();

	AudioGeneratorFrame();
};

class AudioToneGeneratorFrame : public AudioGeneratorFrame {
	GDCLASS(AudioToneGeneratorFrame, AudioGeneratorFrame);

	struct {
		double x[2] = { 0.0, 0.0 };
	} state;
	struct {
		double A[2] = { 0.0, 0.0 };
		double C = 0.0;
	} params;

public:
	virtual void update_parameters() override;
	virtual AudioFrame next() override;
};

class AudioSawGeneratorFrame : public AudioGeneratorFrame {
	GDCLASS(AudioSawGeneratorFrame, AudioGeneratorFrame);

protected:
	struct {
		double x[3] = { 0.0, 0.0, 0.0 };
	} state;
	struct {
		double inc = 0.0;
		double damping = 0.0;
	} params;

public:
	virtual void update_parameters() override;
	virtual AudioFrame next() override;
};

class AudioRectGeneratorFrame : public AudioSawGeneratorFrame {
	GDCLASS(AudioRectGeneratorFrame, AudioSawGeneratorFrame);

public:
	virtual AudioFrame next() override;
};

class AudioVanDerPolGeneratorFrame : public AudioGeneratorFrame {
	GDCLASS(AudioVanDerPolGeneratorFrame, AudioGeneratorFrame);

protected:
	struct {
		double x[3] = { 0.0, 0.0, 0.0 };
	} state;
	struct {
		double phi = 0.0;
		double psi = 0.0;
		double T = 0.0;
		double damping = 0.0;
	} params;

public:
	virtual void update_parameters() override;
	virtual AudioFrame next() override;
};

class AudioStreamToneGeneratorPlayback;

class AudioStreamToneGenerator : public AudioStream {
	GDCLASS(AudioStreamToneGenerator, AudioStream);
	friend class AudioStreamToneGeneratorPlayback;
	friend class AudioToneGeneratorFrame;
	friend class AudioSawGeneratorFrame;
	friend class AudioVanDerPolGeneratorFrame;

	String type;
	float mix_rate;
	float buffer_len;
	float frequency;
	float phase;
	float damping;
	float gain;
	float gain_linear;

	Ref<AudioGeneratorFrame> frame;

protected:
	static void _bind_methods();

public:
	AudioFrame next_frame() const;

	void set_mix_rate(float p_mix_rate);
	float get_mix_rate() const;
	void set_buffer_length(float p_seconds);
	float get_buffer_length() const;
	void set_type(const String &p_type);
	String get_type() const;
	void set_frequency(float p_frequency);
	float get_frequency() const;
	void set_damping(float p_damping);
	float get_damping() const;
	void set_phase(float p_phase);
	float get_phase() const;
	void set_gain(float p_gain);
	float get_gain() const;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual String get_stream_name() const override;

	virtual double get_length() const override;
	virtual bool is_monophonic() const override;

	AudioStreamToneGenerator();
};

class AudioStreamToneGeneratorPlayback : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamToneGeneratorPlayback, AudioStreamPlaybackResampled);
	friend class AudioStreamToneGenerator;
	bool active;
	float mixed;
	AudioStreamToneGenerator *generator;

protected:
	virtual int _mix_internal(AudioFrame *p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

	static void _bind_methods();

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; //times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	AudioStreamToneGeneratorPlayback();
};

#endif // AUDIO_STREAM_TONE_GENERATOR_H
