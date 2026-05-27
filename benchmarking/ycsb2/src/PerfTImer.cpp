#include "PerfTImer.h"

void PerfTimer::start() {
    startTime = std::chrono::steady_clock::now();
}

void PerfTimer::stop() {
    stopTime = std::chrono::steady_clock::now();
}

double PerfTimer::getDuration() const {
    return std::chrono::duration<double>(stopTime - startTime).count();
}

void PerfTimer::setParam(const std::string& name, const std::string& value) {
    params[name] = value;
}

void PerfTimer::setParam(const std::string& name, const char* value) {
    params[name] = value;
}

void PerfTimer::printCounter(std::ostream& headerOut, std::ostream& dataOut, const std::string& name,
    const std::string& counterValue, bool addComma) {
    const auto width = static_cast<int>(std::max(name.length(), counterValue.length()));
    headerOut << std::setw(width) << name << (addComma ? "," : "") << " ";
    dataOut << std::setw(width) << counterValue << (addComma ? "," : "") << " ";
}

void PerfTimer::printReport(std::ostream& out) const {
    std::stringstream header;
    std::stringstream data;
    printReport(header, data);
    out << header.str() << std::endl;
    out << data.str() << std::endl;
}

void PerfTimer::printReport(std::ostream& headerOut, std::ostream& dataOut) const {
    printParams(headerOut, dataOut);
    printCounter(headerOut, dataOut, "duration", getDuration());
}

void PerfTimer::printParams(std::ostream& header, std::ostream& data) const {
    for (auto& p : params) {
        printCounter(header, data, p.first, p.second);
    }
}

PerfTimerBlock::PerfTimerBlock(PerfTimer& timer): timer(timer) {
    timer.start();
}

PerfTimerBlock::~PerfTimerBlock() {
    timer.stop();
    std::stringstream header;
    std::stringstream data;
    timer.printReport(header, data);
    if (timer.printHeader) {
        std::cout << header.str() << std::endl;
        timer.printHeader = false;
    }
    std::cout << data.str() << std::endl;
}
