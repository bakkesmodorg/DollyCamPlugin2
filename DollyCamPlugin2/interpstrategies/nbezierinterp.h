#pragma once
#include "interpstrategy.h"

template <typename T, T(*calc)(T)>
class Mem {
	std::map<T, T> mem_map;

public:
	T operator()(T input) {
		typename std::map<T, T>::iterator it;

		it = mem_map.find(input);
		if (it != mem_map.end()) {
			return it->second;
		}
		else {
			T output = calc(input);
			mem_map[input] = output;
			return output;
		}
	}
};

uint64_t calc_factorial(uint64_t n);

class NBezierInterpStrategy : public InterpStrategy
{
private:
	Mem<uint64_t, calc_factorial> factorial;
public:
	NBezierInterpStrategy(std::shared_ptr<savetype> _camPath, int degree);
	virtual NewPOV GetPOV(float gameTime, int latestFrame);
	virtual std::string GetName();
};

