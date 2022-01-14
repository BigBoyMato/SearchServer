#pragma once

#include <chrono>
#include <iostream>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(X) LogDuration UNIQUE_VAR_NAME_PROFILE(X)
#define LOG_DURATION_STREAM(X, Y) LogDuration UNIQUE_VAR_NAME_PROFILE(X, Y)

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& test_name, std::ostream& ostream)
    	: test_name_(test_name)
    	, ostream_(ostream)
    {
    }

    LogDuration(const std::string& test_name)
    	: LogDuration(test_name, std::cerr)
    {
    }

    LogDuration(std::string_view test_name, std::ostream& ostream)
    	: test_name_(test_name.begin(), test_name.end())
		, ostream_(ostream)
    {
    }

    LogDuration(std::string_view test_name)
    	: LogDuration(test_name, std::cerr)
    {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        ostream_ << test_name_ << " testing..." << std::endl;
        ostream_ << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    std::string test_name_;
    std::ostream& ostream_;

};
