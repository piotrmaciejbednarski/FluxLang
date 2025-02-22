namespace nTest
{
	class cTest
	{
		object oTest
		{
			struct sTest
			{
				string name = "Test";
			} struct_Test;
			
			struct_Test myStruct;
			
			print(myStruct.name);
			
			void main()
			{
				{
					print("Hello");
				};
				{
					print("World!");
				};
			};
		};
	};
};

int{32} main() {
};
