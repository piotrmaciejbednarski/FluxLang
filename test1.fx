object test {
	char y = "First";
};

namespace std
{
	class myClass
	{
		int z = 3;
	};
};

char foo()
{
	print(input("Print before return: "));
	return "The^Fifth.";
};

int main()
{
	test{} a;
	
	print(i"{}\n{}\n{}\n{}":{a.y + " Second!"; a.y + " Third!";"Fourth!";foo()[3];});
	
	return 0;
};
