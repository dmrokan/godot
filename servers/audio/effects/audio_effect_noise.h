/**************************************************************************/
/*  audio_effect_noise.h                                                  */
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

#ifndef AUDIO_EFFECT_NOISE_H
#define AUDIO_EFFECT_NOISE_H

#include "core/math/random_number_generator.h"
#include "servers/audio/audio_effect.h"

class AudioEffectNoise;

class AudioEffectNoiseFrame : public RefCounted {
	GDCLASS(AudioEffectNoiseFrame, RefCounted);
	friend class AudioEffectNoise;

protected:
	struct RNGParams {
		float std;
	};

	Ref<RandomNumberGenerator> rng;
	RNGParams rng_params;
	AudioEffectNoise *effect;

public:
	virtual void update_parameters();
	virtual AudioFrame next();

	AudioEffectNoiseFrame();
};

class AudioEffectWhiteNoiseFrame : public AudioEffectNoiseFrame {
	GDCLASS(AudioEffectWhiteNoiseFrame, AudioEffectNoiseFrame);

public:
	virtual AudioFrame next() override;
};

class AudioEffectBrownNoiseFrame : public AudioEffectNoiseFrame {
	GDCLASS(AudioEffectBrownNoiseFrame, AudioEffectNoiseFrame);

protected:
	struct {
		float x[1];
	} state;

public:
	virtual AudioFrame next() override;

	AudioEffectBrownNoiseFrame();
};

class AudioEffectPinkNoiseFrame : public AudioEffectNoiseFrame {
	GDCLASS(AudioEffectPinkNoiseFrame, AudioEffectNoiseFrame);

	struct State {
		static constexpr int n = 10;
		float x[n + 1];
	} state;
	struct {
		float h[State::n];
	} params;

public:
	virtual AudioFrame next() override;

	AudioEffectPinkNoiseFrame();
};

class AudioEffectVioletNoiseFrame : public AudioEffectBrownNoiseFrame {
	GDCLASS(AudioEffectVioletNoiseFrame, AudioEffectBrownNoiseFrame);

public:
	virtual AudioFrame next() override;
};

class AudioEffectGrayNoiseFrame : public AudioEffectNoiseFrame {
	GDCLASS(AudioEffectGrayNoiseFrame, AudioEffectNoiseFrame);

	struct State {
		static constexpr int n = 7;
		float x[n + 1];
	} state;
	struct {
		float h[State::n];
	} params;

public:
	virtual AudioFrame next() override;

	AudioEffectGrayNoiseFrame();
};

class AudioEffectNoise;

class AudioEffectNoiseInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectNoiseInstance, AudioEffectInstance);
	friend class AudioEffectNoise;
	Ref<AudioEffectNoise> base;

public:
	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;
};

class AudioEffectNoise : public AudioEffect {
	GDCLASS(AudioEffectNoise, AudioEffect);
	friend class AudioEffectNoiseInstance;
	friend class AudioEffectNoiseFrame;

	String type;
	float std;
	float gain;
	float gain_linear;

	Ref<AudioEffectNoiseFrame> frame;

protected:
	static void _bind_methods();

public:
	Ref<AudioEffectInstance> instantiate() override;

	AudioFrame next_frame() const;

	void set_type(const String &p_type);
	String get_type() const;
	void set_std(float p_phase);
	float get_std() const;
	void set_gain(float p_gain);
	float get_gain() const;

	AudioEffectNoise();
};

#endif // AUDIO_EFFECT_NOISE_H
