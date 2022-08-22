#pragma once

#include <vector>

class FrameRateIndicator {
public:
	FrameRateIndicator(int capacity): _capacity(capacity) {
		_frameRates.reserve(capacity);
	}

	~FrameRateIndicator() = default;

	void push(float frameRate) {
		if (_frameRates.size() == _capacity) {
			_frameRates.erase(_frameRates.begin());
		}
		_frameRates.push_back(frameRate);
	}

	float getAverageFrameRate() const {
		float avg = 0.0f;
		for (size_t i = 0; i < _frameRates.size(); ++i) {
			avg += _frameRates[i];
		}

		size_t count = _frameRates.size();
		if (count == 0) {
			count = 1;
		}

		return avg / count;
	}

	const float* getDataPtr() const {
		return _frameRates.data();
	}

	int getSize() const {
		return static_cast<int>(_frameRates.size());
	}
private:
	std::vector<float> _frameRates;
	const int _capacity;
};