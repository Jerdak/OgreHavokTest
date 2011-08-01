#ifndef __SINGLETON_TEMPLATE__
#define __SINGLETON_TEMPLATE__

#pragma once

#include <iostream>
 
namespace HK {
	template<typename T> class Singleton
	{
	  public:
		static T& GetSingleton()
		{
			static T theInstance;  // assumes T has a protected default constructor
			return theInstance;
		}
	};
	 
	//Example Class
	class OnlyOne : public Singleton<OnlyOne>
	{
		friend class Singleton<OnlyOne>;
		int example_data;
		public:
			int Getexample_data() const {return example_data;}
		protected: 
			OnlyOne(): example_data(42) {}   // default constructor 
	};
};

#endif //__SINGLETON_TEMPLATE__