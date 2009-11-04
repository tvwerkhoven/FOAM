#ifndef __FORMAT_H__
#define __FORMAT_H__

#include <cstdarg>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <cstdlib>

static std::string popword(std::string &line) {
	size_t b, e = 0;

	// Strip initial whitespace
	b = line.find_first_not_of(" \t\n", 0);
	if(b != std::string::npos)
		line.erase(0, b);

	// If it starts with :, use the rest of the line
	if(line[b] == ':') {
		std::string result = line;
		result.erase(0, 1);
		line.erase();
		return result;
	}

	// Otherwise find the next whitespace
	e = line.find_first_of(" \t\n", 0);
	if(e == std::string::npos)
		e = line.size();

	// Save result
	std::string result(line, 0, e);

	// Erase until next word
	e = line.find_first_not_of(" \t\n", e);
	if(e == std::string::npos)
		e = line.size();

	line.erase(0, e);

	return result;
}

static inline bool popstatus(std::string &line) { return popword(line) == "OK"; }
static inline void popstatus(std::string &line, const std::string &errormsg) { if(!popstatus(line)) throw std::runtime_error(errormsg); }
static inline bool popexpect(std::string &line, const std::string &expect) { return popword(line) == expect; }
static inline void popexpect(std::string &line, const std::string &expect, const std::string &errormsg) { if(!popexpect(line, expect)) throw std::runtime_error(errormsg); }

static inline double str2double(const std::string &line) { return atof(line.c_str()); }
static inline int str2int(const std::string &line) { return atoi(line.c_str()); }
static inline int32_t str2int32(const std::string &line) { return atoi(line.c_str()); }

static inline double popdouble(std::string &line) { return str2double(popword(line)); }
static inline int popint(std::string &line) { return str2int(popword(line)); }
static inline int32_t popint32(std::string &line) { return str2int32(popword(line)); }

static inline std::string vformat(const char *format, va_list va) {
	char buf[4096];
	vsnprintf(buf, sizeof buf, format, va);
	return std::string(buf);
}

static inline std::string format(const char *format, ...) {
	va_list va;
	va_start(va, format);
	std::string result = vformat(format, va);
	va_end(va);
	return result;
}

#endif
