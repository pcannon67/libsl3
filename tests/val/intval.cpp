
#include <a4testing.hpp>

#include <sl3/value.hpp>
#include <sl3/error.hpp>
#include "eqorder.hpp"
#include <functional>

namespace sl3
{
  namespace
  {

    using VauleRelation = std::function<bool(const Value&, const Value&)>;

    VauleRelation strong_eq = std::equal_to<sl3::Value>() ;
    VauleRelation strong_lt = std::less<Value>() ;

    VauleRelation weak_eq = sl3::weak_eq;
    VauleRelation weak_lt = sl3::weak_lt ;
  
    void
    create ()
    {
      Value a(100);
      Value b(a);
      Value c = a ;
      Value d = 100 ;

      BOOST_CHECK_EQUAL (a, 100) ;
      BOOST_CHECK_EQUAL (a.getType(), Type::Int) ;
      BOOST_CHECK_EQUAL (b, 100) ;
      BOOST_CHECK_EQUAL (b.getType(), Type::Int) ;
      BOOST_CHECK_EQUAL (c, 100) ;
      BOOST_CHECK_EQUAL (c.getType(), Type::Int) ;
      BOOST_CHECK_EQUAL (d, 100) ;
      BOOST_CHECK_EQUAL (d.getType(), Type::Int) ;

      Value e;
      BOOST_CHECK (e.isNull()) ;
      BOOST_CHECK_EQUAL (e.getType(), Type::Null) ;
      e = 100 ;
      BOOST_CHECK_EQUAL (e, 100) ;
      BOOST_CHECK_EQUAL (e.getType(), Type::Int) ;
      BOOST_CHECK (!e.isNull()) ;

    }


    void
    assing ()
    {

      Value a;
      BOOST_CHECK (a.isNull()) ;
      BOOST_CHECK_EQUAL (a.getType(), Type::Null) ;
      int64_t intval = 100 ;
      a = intval ;
      BOOST_CHECK_EQUAL (a , intval) ;
      BOOST_CHECK_EQUAL (a.getType(), Type::Int) ;
      BOOST_CHECK (!a.isNull()) ;

      Value b;
      b = a ;
      BOOST_CHECK_EQUAL (b , intval) ;
      BOOST_CHECK_EQUAL (b , a) ;

    }

    void
    move()
    {
      Value a{1};
      BOOST_CHECK (!a.isNull()) ;
      BOOST_CHECK_EQUAL (a.getType(), Type::Int) ;

      Value b{std::move(a)};
      BOOST_CHECK (a.isNull()) ;
      BOOST_CHECK_EQUAL (a.getType(), Type::Null) ;

      BOOST_CHECK_EQUAL (b.getType(), Type::Int) ;
      BOOST_CHECK_EQUAL (b, 1) ;

      Value c = std::move(b);
      BOOST_CHECK (b.isNull()) ;

      BOOST_CHECK_EQUAL (c.getType(), Type::Int) ;
      BOOST_CHECK_EQUAL (c, 1) ;

      Value d;
      d = std::move(c);

      BOOST_CHECK (c.isNull()) ;
      BOOST_CHECK_EQUAL (d.getType(), Type::Int) ;
      BOOST_CHECK_EQUAL (d, 1) ;


    }

    void
     equality ()
     {
        Value a(100), b(100), c(100) ;
        using namespace eqo ;

        BOOST_CHECK (eq_reflexive (a, strong_eq));
        BOOST_CHECK (weak_reflexive (a,b, weak_eq));

        BOOST_CHECK (eq_symmetric (a,b, strong_eq));
        BOOST_CHECK (eq_symmetric (a,b, weak_eq));

        BOOST_CHECK (eq_transitive (a,b,c, strong_eq));
        BOOST_CHECK (eq_transitive (a,b,c, weak_eq));

     }



     void
     strictTotalOrdered ()
     {
       using namespace eqo ;

       BOOST_CHECK (strong_eq(Value{1}, Value{1}));
       BOOST_CHECK (!strong_eq(Value{1}, Value{1.0}));

       Value a(100), b(100), c(100) ;
       Value d(100), e(200), f(300) ;

       BOOST_CHECK (eq_reflexive (a, strong_eq));
       BOOST_CHECK (eq_symmetric (a,b, strong_eq));
       BOOST_CHECK (eq_transitive (a,b,c, strong_eq));

       BOOST_CHECK (irreflexive (a,b, strong_eq, strong_lt));
       BOOST_CHECK (lt_transitive (d,e,f, strong_lt));
       BOOST_CHECK (trichotomy (d,e, strong_eq, strong_lt));


     }


     void
     weakTotalOrdered ()
     {
       using namespace eqo ;
       BOOST_CHECK (weak_eq(Value{1}, Value{1}));
       BOOST_CHECK (weak_eq(Value{1}, Value{1.0}));

       Value a(100), b(100.0), c(100) ;
       Value d(100), e(200.0), f(300) ;

       BOOST_CHECK (weak_reflexive (a,a, weak_eq));
       BOOST_CHECK (eq_symmetric (a,b, weak_eq));
       BOOST_CHECK (eq_transitive (a,b,c, weak_eq));

       BOOST_CHECK (irreflexive (a,b, weak_eq, weak_lt));
       BOOST_CHECK (lt_transitive (d,e,f, weak_lt));
       BOOST_CHECK (trichotomy (d,e, weak_eq, weak_lt));

     }



     void
     compareWithOthers ()
     {
       Value nullVal ;
       Value intVal (1) ;
       Value realVal (1.0) ;
       Value textVal ("a");
       Value blobVal (Blob{});

       BOOST_CHECK (intVal != nullVal);
       BOOST_CHECK (intVal > nullVal);

       BOOST_CHECK (intVal != realVal);
       BOOST_CHECK (intVal < realVal);

       BOOST_CHECK (intVal != textVal);
       BOOST_CHECK (intVal < textVal);

       BOOST_CHECK (intVal != blobVal);
       BOOST_CHECK (intVal < blobVal);

     }


    a4TestAdd (a4test::suite ("intval")
          .addTest ("create", create)
          .addTest ("assing", assing)
          .addTest ("move", move)
          .addTest ("equality", equality)
          .addTest ("strictTotalOrdered", strictTotalOrdered)
          .addTest ("weakTotalOrdered", weakTotalOrdered)
          .addTest ("compareWithOthers", compareWithOthers)

                   );
  }
}

