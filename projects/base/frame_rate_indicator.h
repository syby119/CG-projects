#pragma once

#include <vector>

class FrameRateIndicator {
public:
    FrameRateIndicator(int capacity) : m_capacity(capacity) {
        m_frameRates.reserve(capacity);
    }

    ~FrameRateIndicator() = default;

    void push(float frameRate) {
        if (m_frameRates.size() == m_capacity) {
            m_frameRates.erase(m_frameRates.begin());
        }
        m_frameRates.push_back(frameRate);
    }

    float getAverageFrameRate() const {
        float avg = 0.0f;
        for (size_t i = 0; i < m_frameRates.size(); ++i) {
            avg += m_frameRates[i];
        }

        size_t count = m_frameRates.size();
        if (count == 0) {
            count = 1;
        }

        return avg / count;
    }

    const float* getDataPtr() const {
        return m_frameRates.data();
    }

    int getSize() const {
        return static_cast<int>(m_frameRates.size());
    }

private:
    std::vector<float> m_frameRates;
    const int m_capacity;
};