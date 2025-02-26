object test {
	int x = 5;
	
	void foo()
	{
		return "Hello World!";
	};
};

int main()
{
	test{} a;
	
	return a.foo();
};
