/**************************************************************************/
/*  audio_effect_generator.h                                              */
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

#ifndef AUDIO_EFFECT_GENERATOR_H
#define AUDIO_EFFECT_GENERATOR_H

#include "core/math/random_number_generator.h"
#include "servers/audio/audio_effect.h"

class AudioEffectGenerator;

class AudioEffectBaseGeneratorInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectBaseGeneratorInstance, AudioEffectInstance);
	friend class AudioEffectGeneratorInstance;
	float gain;
	float offset;

public:
	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;

	virtual float get_output();
};

class AudioEffectToneGeneratorInstance : public AudioEffectBaseGeneratorInstance {
	GDCLASS(AudioEffectToneGeneratorInstance, AudioEffectBaseGeneratorInstance);
	friend class AudioEffectGeneratorInstance;
	struct State {
		double x[2];
	};
	struct Params {
		double A[2];
		double C;
	};

	State state;
	Params params;

public:
	virtual float get_output() override;
};

class AudioEffectSawGeneratorInstance : public AudioEffectBaseGeneratorInstance {
	GDCLASS(AudioEffectSawGeneratorInstance, AudioEffectBaseGeneratorInstance);
	friend class AudioEffectGeneratorInstance;
	struct State {
		double x[1];
	};
	struct Params {
		double inc;
	};

	State state;
	Params params;

public:
	virtual float get_output() override;
};

class AudioEffectWhiteNoiseGeneratorInstance : public AudioEffectBaseGeneratorInstance {
	GDCLASS(AudioEffectWhiteNoiseGeneratorInstance, AudioEffectBaseGeneratorInstance);
	friend class AudioEffectGeneratorInstance;

protected:
	struct Params {
		float mean;
		float std;
	};

	Ref<RandomNumberGenerator> rng;
	Params params;

public:
	virtual float get_output() override;

	AudioEffectWhiteNoiseGeneratorInstance();
};

class AudioEffectBrownNoiseGeneratorInstance : public AudioEffectWhiteNoiseGeneratorInstance {
	GDCLASS(AudioEffectBrownNoiseGeneratorInstance, AudioEffectWhiteNoiseGeneratorInstance);
	friend class AudioEffectGeneratorInstance;
	struct State {
		float x[1];
	};

	State state;

public:
	virtual float get_output() override;
};

class AudioEffectPinkNoiseGeneratorInstance : public AudioEffectWhiteNoiseGeneratorInstance {
	GDCLASS(AudioEffectPinkNoiseGeneratorInstance, AudioEffectWhiteNoiseGeneratorInstance);
	friend class AudioEffectGeneratorInstance;
	struct State {
		static constexpr int n = 10;
		float x[n + 1];
	};
	struct Filter {
		float h[State::n];

		Filter();
	};

	State state;
	Filter filter;

public:
	virtual float get_output() override;

	void reset_state();
};

class AudioEffectGeneratorInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectGeneratorInstance, AudioEffectInstance);
	friend class AudioEffectGenerator;
	enum GeneratorType {
		TYPE_BASE = 0,
		TYPE_TONE,
		TYPE_SAW,
		TYPE_WHITE_NOISE,
		TYPE_BROWN_NOISE,
		TYPE_PINK_NOISE,
	};

	struct {
		Ref<AudioEffectToneGeneratorInstance> tone;
		Ref<AudioEffectSawGeneratorInstance> saw;
		Ref<AudioEffectWhiteNoiseGeneratorInstance> white_noise;
		Ref<AudioEffectBrownNoiseGeneratorInstance> brown_noise;
		Ref<AudioEffectPinkNoiseGeneratorInstance> pink_noise;
	} generator_instance;
	Ref<AudioEffectGenerator> base;
	GeneratorType generator_type;

	template <typename T>
	Ref<T> get_generator_insance() {
		switch (generator_type) {
			case TYPE_SAW:
				if (generator_instance.saw.is_null())
					generator_instance.saw.instantiate();
				return generator_instance.saw;
			case TYPE_WHITE_NOISE:
				if (generator_instance.white_noise.is_null())
					generator_instance.white_noise.instantiate();
				return generator_instance.white_noise;
			case TYPE_BROWN_NOISE:
				if (generator_instance.brown_noise.is_null())
					generator_instance.brown_noise.instantiate();
				return generator_instance.brown_noise;
			case TYPE_PINK_NOISE:
				if (generator_instance.pink_noise.is_null()) {
					generator_instance.pink_noise.instantiate();
					generator_instance.pink_noise->filter = AudioEffectPinkNoiseGeneratorInstance::Filter();
				}
				return generator_instance.pink_noise;
			case TYPE_TONE:
			default:
				if (generator_instance.tone.is_null())
					generator_instance.tone.instantiate();
				return generator_instance.tone;
				break;
		}

		return nullptr;
	}

public:
	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;

	void set_generator_type(GeneratorType type);
	void update_parameters();
	void update_initial_conditions();
	void parameters_updated();

	AudioEffectGeneratorInstance();
};

class AudioEffectGenerator : public AudioEffect {
	GDCLASS(AudioEffectGenerator, AudioEffect);
	friend class AudioEffectGeneratorInstance;

	struct Params {
		float sampling_frequency;
		float frequency;
		float damping;
		float phase;
		float gain_db;
		float gain_linear;
		float offset;
		float mean;
		float std;
		AudioEffectGeneratorInstance::GeneratorType type;
	};

	Params params;
	String s_type;
	bool is_parameters_updated;
	bool should_reset_state;
	bool is_type_updated;

protected:
	static void _bind_methods();

public:
	Ref<AudioEffectInstance> instantiate() override;
	void set_sampling_frequency(float p_sampling_frequency);
	float get_sampling_frequency();
	void set_frequency(float p_frequency);
	float get_frequency() const;
	void set_damping(float p_damping);
	float get_damping() const;
	void set_phase(float p_phase);
	float get_phase() const;
	void set_gain_db(float p_gain_db);
	float get_gain_db() const;
	void set_offset(float p_offset);
	float get_offset() const;
	void set_type(const String &p_type);
	String get_type() const;
	void set_mean(float p_mean);
	float get_mean() const;
	void set_std(float p_std);
	float get_std() const;
	void reset();

	AudioEffectGenerator();
};

#endif // AUDIO_EFFECT_GENERATOR_H
