/*
 sigcpp-test.cc -- proper example of using derived classes and virtual 
 functions (with sigcpp as specific example).
 
 Tim van Werkhoven, 20101223
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
