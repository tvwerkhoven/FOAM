/*
 sigcpp-test.cc -- proper example of using derived classes and virtual functions (with sigcpp as specific example).
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <sigc++/signal.h>
#include <string>

using namespace std;

class SendSig{
public:
	SendSig() {
		printf("SendSig::SendSig()\n");
	}
	~SendSig() {
		printf("SendSig::~SendSig()\n");
	}
	void run() {
		printf("SendSig::Run()\n");
		signal_detected();
	}
	sigc::signal<void> signal_detected;	// changed
};

class Base {
public:
	SendSig &sig;
	Base(SendSig &sig): sig(sig) {
		printf("Base::Base()\n");
		sig.signal_detected.connect(sigc::mem_fun(*this, &Base::callback));
	}
	virtual ~Base() {
		printf("Base::~Base()\n");
	}
	virtual void callback() {
//		vfunc("vfunc");
//		Base::vfunc("Base::vfunc");
		printf("Base::callback()\n");
	}
	virtual void vfunc(string s) {
		printf("Base::vfunc(s=%s)\n", s.c_str());
	}
};

class Super : public Base {
public:
	Super(SendSig &sig): Base(sig) {
		printf("Super::Super()\n");
	}
	~Super() {
		printf("Super::~Super()\n");
	}
	virtual void callback() {
		Base::callback();
		printf("Super::callback()\n");
	}
	virtual void vfunc(string s) {
		printf("Super::vfunc(s=%s)\n", s.c_str());
	}
};

int main() {
	printf("sigc++-test\n\n");
	SendSig sig;
	
//	Base b1(sig);
//	sig.run();
	
	Super s1(sig);
	s1.callback();
	
	sig.run();


	
	return 0;
}
