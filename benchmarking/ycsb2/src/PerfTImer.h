#pragma once

/**
 * This serves the same purpose as BtreeCppPerfEvent.hpp, except all linux kernel profiling calls have been removed.
 * The only metrics remaining are time measuring.
 */

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <map>

class PerfTimer {
public:
    void start();
    void stop();

    [[nodiscard]] double getDuration() const;

    void setParam(const std::string& name, const std::string& value);
    void setParam(const std::string& name, const char* value);
    template <typename T>
    void setParam(const std::string& name, T value);

private:
    static void printCounter(std::ostream& headerOut, std::ostream& dataOut, const std::string& name, const std::string& counterValue, bool addComma = true);
    template <typename T>
    static void printCounter(std::ostream& headerOut, std::ostream& dataOut, const std::string& name, T counterValue, const bool addComma = true);
    void printReport(std::ostream& out) const;
    void printReport(std::ostream& headerOut, std::ostream& dataOut) const;
    void printParams(std::ostream& header, std::ostream& data) const;

    friend class PerfTimerBlock;

    std::map<std::string, std::string> params;
    bool printHeader = true;

    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::time_point<std::chrono::steady_clock>::min();
    std::chrono::time_point<std::chrono::steady_clock> stopTime = std::chrono::time_point<std::chrono::steady_clock>::min();

};

template <typename T>
void PerfTimer::setParam(const std::string& name, T value) {
    setParam(name, std::to_string(value));
}

template <typename T>
void PerfTimer::printCounter(std::ostream& headerOut, std::ostream& dataOut, const std::string& name, T counterValue,
    const bool addComma) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(3) << counterValue;
    printCounter(headerOut, dataOut, name, stream.str(), addComma);
}

class PerfTimerBlock {
public:
    explicit PerfTimerBlock(PerfTimer& timer);
    ~PerfTimerBlock();

private:
    PerfTimer& timer;
};

#include "config.hpp"

inline PerfTimer makePerfTimer(const std::string& dataName, const bool dataSorted, const unsigned dataSize)
{
    PerfTimer timer;
    timer.setParam("op", "none");
    timer.setParam("config_name", configName);

    timer.setParam("data_name", dataName);
    timer.setParam("data_sorted", dataSorted);
    timer.setParam("data_size", dataSize);
    for (auto x : btree_constexpr_settings) {
        timer.setParam(x.first, std::to_string(x.second));
    }

    return timer;
}