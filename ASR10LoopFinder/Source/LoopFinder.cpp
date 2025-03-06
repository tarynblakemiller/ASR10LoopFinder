/*
 ==============================================================================
 
 LoopFinder.cpp
 Created: 6 Mar 2025 2:13:47am
 Author:  Taryn Blake Miller
 
 ==============================================================================
 */
#include "LoopFinder.h"

LoopFinder::LoopFinder(const juce::AudioSampleBuffer& buffer, double sampleRate)
    : buffer(buffer), sampleRate(sampleRate) {}

LoopFinder::ZeroCrossingType LoopFinder::getZeroCrossingType(int channel, int index) const {
    const float* channelData = buffer.getReadPointer(channel);
    int length = buffer.getNumSamples();
    if (index == 0 || index == length - 1) return LoopFinder::ZeroCrossingType::None;
    float prev = channelData[index - 1];
    float curr = channelData[index];
    if (prev > 0 && curr <= 0) return LoopFinder::ZeroCrossingType::Down;
    if (prev < 0 && curr >= 0) return LoopFinder::ZeroCrossingType::Up;
    return LoopFinder::ZeroCrossingType::None;
}

std::vector<int> LoopFinder::findZeroCrossings(int channel) const {
    const float* channelData = buffer.getReadPointer(channel);
    int length = buffer.getNumSamples();
    std::vector<int> crossings;
    for (int i = 1; i < length - 1; ++i) {
        if (channelData[i-1] * channelData[i] <= 0) {
            crossings.push_back(i);
        }
    }
    return crossings;
}

std::vector<int> LoopFinder::findCommonZeroCrossings(const std::vector<int>& leftCrossings, const std::vector<int>& rightCrossings) const {
    std::vector<int> common;
    auto itLeft = leftCrossings.begin();
    auto itRight = rightCrossings.begin();
    while (itLeft != leftCrossings.end() && itRight != rightCrossings.end()) {
        if (*itLeft == *itRight) {
            common.push_back(*itLeft);
            ++itLeft;
            ++itRight;
        } else if (*itLeft < *itRight) {
            ++itLeft;
        } else {
            ++itRight;
        }
    }
    return common;
}

std::vector<std::pair<int, LoopFinder::ZeroCrossingType>> LoopFinder::getAllZeroCrossings() {
    auto leftCrossings = findZeroCrossings(0);
    auto rightCrossings = findZeroCrossings(1);
    auto commonCrossings = findCommonZeroCrossings(leftCrossings, rightCrossings);
    std::vector<std::pair<int, LoopFinder::ZeroCrossingType>> typedCrossings;
    for (int crossing : commonCrossings) {
        LoopFinder::ZeroCrossingType type = getZeroCrossingType(0, crossing);
        if (type != LoopFinder::ZeroCrossingType::None) {
            typedCrossings.push_back({crossing, type});
        }
    }
    return typedCrossings;
}

std::pair<int, int> LoopFinder::findDefaultLoopPoints() {
    int sampleLength = buffer.getNumSamples();
    int startIdx = static_cast<int>(0.25 * sampleLength);
    auto typedCrossings = getAllZeroCrossings();

    int startPoint = -1;
    for (const auto& pair : typedCrossings) {
        if (pair.first >= startIdx && pair.second == LoopFinder::ZeroCrossingType::Down) {
            startPoint = pair.first;
            break;
        }
    }
    if (startPoint == -1) return {-1, -1};

    int minCycleLength = static_cast<int>(sampleRate / 1000.0);
    int maxCycleLength = static_cast<int>(sampleRate / 50.0);
    int bestEnd = -1;
    float bestScore = INFINITY;

    for (const auto& pair : typedCrossings) {
        if (pair.first > startPoint && pair.second == LoopFinder::ZeroCrossingType::Down) {
            int loopLength = pair.first - startPoint;
            if (loopLength >= minCycleLength && loopLength <= maxCycleLength) {
                float score = std::abs(buffer.getSample(0, pair.first - 1) - buffer.getSample(0, startPoint - 1)) +
                              std::abs(buffer.getSample(1, pair.first - 1) - buffer.getSample(1, startPoint - 1));
                if (score < bestScore) {
                    bestScore = score;
                    bestEnd = pair.first;
                }
            }
        }
    }

    if (bestEnd == -1) {
        int minEnd = startPoint + minCycleLength;
        bestEnd = (minEnd < sampleLength) ? minEnd : sampleLength - 1;
    }

    return {startPoint, bestEnd};
}
