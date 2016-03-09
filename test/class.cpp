/****************************************************************************
**
** This file is part of the Ponder library, formerly CAMP.
**
** The MIT License (MIT)
**
** Copyright (C) 2009-2014 TEGESO/TEGESOFT and/or its subsidiary(-ies) and mother company.
** Contact: Tegesoft Information (contact@tegesoft.com)
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#include <ponder/classget.hpp>
#include <ponder/pondertype.hpp>
#include <ponder/class.hpp>
#include <ponder/classbuilder.hpp>
#include "catch.hpp"

namespace ClassTest
{
    struct MyExplicityDeclaredClass
    {
    };
    
    struct MyUndeclaredClass
    {
    };
    
    struct MyClass
    {
        void func() {}
        int prop;
    };
    
    struct MyClass2
    {
    };
    
    struct Base
    {
        virtual ~Base() {}
        PONDER_RTTI();
    };
    
    struct Derived : Base
    {
        PONDER_RTTI();
    };
    
    struct DerivedNoRtti : Base
    {
    };
    
    struct Derived2NoRtti : Derived
    {
    };
    
    void declare()
    {
        ponder::Class::declare<MyClass>("ClassTest::MyClass")
            .property("prop", &MyClass::prop)
            .function("func", &MyClass::func);
        
        ponder::Class::declare<MyClass2>("ClassTest::MyClass2");
        
        ponder::Class::declare<Base>("ClassTest::Base");
        
        ponder::Class::declare<Derived>("ClassTest::Derived")
            .base<Base>();
        
        ponder::Class::declare<DerivedNoRtti>("ClassTest::DerivedNoRtti")
            .base<Base>();
        
        ponder::Class::declare<Derived2NoRtti>("ClassTest::Derived2NoRtti")
            .base<Derived>();
    }
}

PONDER_TYPE(ClassTest::MyExplicityDeclaredClass /* never declared */)
PONDER_TYPE(ClassTest::MyUndeclaredClass /* never declared */)

//
// ClassTest::declare() is called to declared registered classes in it. Note,
// function will only be called once, by the first class that is required. If
// re-registered (a duplicate), this would throw an exception.
//
PONDER_AUTO_TYPE(ClassTest::MyClass, &ClassTest::declare)
PONDER_AUTO_TYPE(ClassTest::MyClass2, &ClassTest::declare)
PONDER_AUTO_TYPE(ClassTest::Base, &ClassTest::declare)
PONDER_AUTO_TYPE(ClassTest::Derived, &ClassTest::declare)
PONDER_AUTO_TYPE(ClassTest::DerivedNoRtti, &ClassTest::declare)
PONDER_AUTO_TYPE(ClassTest::Derived2NoRtti, &ClassTest::declare)

using namespace ClassTest;

//-----------------------------------------------------------------------------
//                         Tests for ponder::Class
//-----------------------------------------------------------------------------

TEST_CASE("Classes need to be declared")
{
    SECTION("explicit declaration")
    {
        const std::size_t count = ponder::classCount();    
        ponder::Class::declare<MyExplicityDeclaredClass>("ClassTest::MyExplicityDeclaredClass");
        REQUIRE(ponder::classCount() == count + 1);        
    }
    
    SECTION("duplicates are errors")
    {
        ponder::classByType<MyClass>(); // to make sure it is declared
    
        // deplicate by type
        REQUIRE_THROWS_AS(ponder::Class::declare<MyClass>(), ponder::ClassAlreadyCreated);
        
        // duplicate by name
        REQUIRE_THROWS_AS(ponder::Class::declare<MyUndeclaredClass>("ClassTest::MyClass"),
                          ponder::ClassAlreadyCreated);        
    }
    
    SECTION("metadata can be compared")
    {
        const ponder::Class& class1 = ponder::classByType<MyClass>();
        const ponder::Class& class2 = ponder::classByType<MyClass2>();
        
        REQUIRE(class1 == class1);
        REQUIRE(class1 != class2);
        REQUIRE(class2 != class1);
    }    
}


TEST_CASE("Class metadata can be retrieved")
{
    MyClass object;
    MyUndeclaredClass object2;    

    SECTION("by name")
    {
        REQUIRE(ponder::classByName("ClassTest::MyClass").name() == "ClassTest::MyClass");        
        
        REQUIRE_THROWS_AS(ponder::classByName("ClassTest::MyUndeclaredClass"), 
                          ponder::ClassNotFound);
    }
    
    SECTION("by type")
    {
        REQUIRE(ponder::classByType<MyClass>().name() == "ClassTest::MyClass");
        REQUIRE(ponder::classByTypeSafe<MyUndeclaredClass>() == static_cast<ponder::Class*>(0));    
        
        REQUIRE_THROWS_AS(ponder::classByType<MyUndeclaredClass>(),            
                          ponder::ClassNotFound);
    }

    SECTION("by instance")
    {
        REQUIRE(ponder::classByObject(object).name() == "ClassTest::MyClass");
        REQUIRE(ponder::classByObject(&object).name() == "ClassTest::MyClass");
        
        REQUIRE_THROWS_AS(ponder::classByObject(object2), ponder::ClassNotFound);
        REQUIRE_THROWS_AS(ponder::classByObject(&object2), ponder::ClassNotFound);
    }    
}


TEST_CASE("Class members can be inspected")
{
    const ponder::Class& metaclass = ponder::classByType<MyClass>();

    SECTION("can have properties")
    {
        REQUIRE(metaclass.propertyCount() == 1U);
        REQUIRE(metaclass.hasProperty("prop") == true);
        REQUIRE(metaclass.hasProperty("xxxx") == false);        
    }
    
    SECTION("can have functions")
    {
        REQUIRE(metaclass.functionCount() == 1U);
        REQUIRE(metaclass.hasFunction("func") == true);
        REQUIRE(metaclass.hasFunction("xxxx") == false);
    }
}


TEST_CASE("Classes can use inheritance")
{
    const ponder::Class& derived = ponder::classByType<Derived>();

    REQUIRE(derived.baseCount() == 1U);
    REQUIRE(derived.base(0).name() == "ClassTest::Base");
    REQUIRE_THROWS_AS(derived.base(1), ponder::OutOfRange);
}


TEST_CASE("Classes can have hierarchies")
{
    Base* base    = new Base;
    Base* derived = new Derived;
    Base* nortti  = new DerivedNoRtti;
    Base* nortti2 = new Derived2NoRtti;

    REQUIRE(ponder::classByObject(base).name() == "ClassTest::Base");    // base is really a base
    REQUIRE(ponder::classByObject(*base).name() == "ClassTest::Base");
    
    SECTION("with rtti")
    {
        // Ponder finds its real type thanks to PONDER_RTTI
        REQUIRE(ponder::classByObject(derived).name() == "ClassTest::Derived");
        REQUIRE(ponder::classByObject(*derived).name() == "ClassTest::Derived");        
    }
    
    SECTION("without rtti")
    {
        // Ponder fails to find its derived type without PONDER_RTTI
        REQUIRE(ponder::classByObject(nortti).name() == "ClassTest::Base");
        REQUIRE(ponder::classByObject(*nortti).name() == "ClassTest::Base");
    }

   SECTION("allows polymorphism")
   {
       Base* genericBase = derived;
       REQUIRE(ponder::classByObject(genericBase).name() == "ClassTest::Derived");
       REQUIRE(ponder::classByObject(*genericBase).name() == "ClassTest::Derived");
   }
    
    SECTION("without rtti, no polymorphism")
    {
        Base* nonGenericBase = nortti;
        // Ponder fails to find its derived type without PONDER_RTTI
        REQUIRE(ponder::classByObject(nonGenericBase).name() == "ClassTest::Base");
        REQUIRE(ponder::classByObject(*nonGenericBase).name() == "ClassTest::Base");
    }

    // REQUIRE(ponder::classByObject(nortti2).name(),  "ClassTest::Derived"); // Ponder finds the closest derived type which has PONDER_RTTI
    // REQUIRE(ponder::classByObject(*nortti2).name(), "ClassTest::Derived");

    delete nortti2;
    delete nortti;
    delete derived;
    delete base;
}


//TEST_CASE(typeNames)
//{
//    BOOST_CHECK(strcmp(ponder::detail::typeAsString(ponder::noType), "none")==0);
//    BOOST_CHECK(strcmp(ponder::detail::typeAsString(ponder::realType), "real")==0);
//    BOOST_CHECK(strcmp(ponder::detail::typeAsString(ponder::userType), "user")==0);
//}

