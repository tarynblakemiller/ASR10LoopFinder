/*
 ==============================================================================
 
 LoopFinder.h
 Created: 6 Mar 2025 2:13:47am
 Author:  Taryn Blake Miller
 
 ==============================================================================
 */

//#pragma once
#ifndef LOOPFINDER_H
#define LOOPFINDER_H

#include <JuceHeader.h>

class LoopFinder {
public:
    enum class ZeroCrossingType { Up, Down, None };
    
    LoopFinder(const juce::AudioSampleBuffer& buffer, double sampleRate);
    std::vector<std::pair<int, ZeroCrossingType>> getAllZeroCrossings();
    std::pair<int, int> findDefaultLoopPoints();
    
private:
    ZeroCrossingType getZeroCrossingType(int channel, int index) const;
    std::vector<int> findZeroCrossings(int channel) const;
    std::vector<int> findCommonZeroCrossings(const std::vector<int>& leftCrossings, const std::vector<int>& rightCrossings) const;
    //    std::vector<std::pair<int, ZeroCrossingType>> getCommonZeroCrossingsWithTypes() const;
    //    int findStartPoint(const std::vector<std::pair<int, ZeroCrossingType>>& typedCrossings, int startIdx) const;
    //    std::vector<int> findPotentialEndPoints(const std::vector<std::pair<int, ZeroCrossingType>>& typedCrossings, int startPoint) const;
    //    float calculateScore(int start, int end) const;
    //    int findBestEndPoint(int start, const std::vector<int>& potentialEnds, int minCycleLength, int maxCycleLength) const;
    
    const juce::AudioSampleBuffer& buffer;
    double sampleRate;
    
};

#endif
