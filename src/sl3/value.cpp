/******************************************************************************
 ------------- Copyright (c) 2009-2017 H a r a l d  A c h i t z ---------------
 ---------- < h a r a l d dot a c h i t z at g m a i l dot c o m > ------------
 ---- This Source Code Form is subject to the terms of the Mozilla Public -----
 ---- License, v. 2.0. If a copy of the MPL was not distributed with this -----
 ---------- file, You can obtain one at http://mozilla.org/MPL/2.0/. ----------
 ******************************************************************************/

#include <sl3/error.hpp>
#include <sl3/value.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <climits>
#include <ostream>
#include <type_traits>

namespace sl3
{
  namespace
  {
    // not in coverage, only used in never reachable case/if branches
    void raiseErrUnexpected (const std::string& msg) // LCOV_EXCL_LINE
    {
      throw ErrUnexpected (msg); // LCOV_EXCL_LINE
    }

    template <class T>
    typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
    almost_equal (T x, T y, int ulp)
    {
      using std::numeric_limits;
      // the machine epsilon has to be scaled to the magnitude of the values
      // used
      // and multiplied by the desired precision in ULPs (units in the last
      // place)
      return std::abs (x - y)
                 < numeric_limits<T>::epsilon () * std::abs (x + y) * ulp
             // unless the result is subnormal
             || std::abs (x - y) < numeric_limits<T>::min ();
    }

    template <typename InT, typename OutT>
    // requires std::is_integral<OutT>::value
    // requires is_floating_point<InT>::value
    inline OutT
    losslessConvert1 (InT in)
    {
      InT converted = std::trunc (in);
      if (in - converted != 0.0)
        throw ErrTypeMisMatch{"Conversion loses fraction"};

      using limit = std::numeric_limits<OutT>;
      if (converted < limit::min () || converted > limit::max ())
        throw ErrOutOfRange{"Converted value to big"};

      return static_cast<OutT> (converted);
    }

    template <typename InT, typename OutT>
    // requires std::is_integral<OutT>::value
    // requires is_floating_point<InT>::value
    inline OutT
    losslessConvert2 (InT in)
    {
      InT converted{0.0};
      InT fraction = std::modf (in, &converted);

      if (fraction != 0.0)
        throw ErrOutOfRange{"Conversion loses fraction"};

      using limit = std::numeric_limits<OutT>;
      if (converted < limit::min () || converted > limit::max ())
        throw ErrOutOfRange{"Converted value to big"};

      return static_cast<OutT> (converted);
    }

  } //--------------------------------------------------------------------------

  Value::Value () noexcept
  : _type (Type::Null)
  {
  }

  Value::Value (int val) noexcept
  : _type (Type::Int)
  {
    _store.intval = val;
  }

  Value::Value (int64_t val) noexcept
  : _type (Type::Int)
  {
    _store.intval = val;
  }

  Value::Value (std::string val) noexcept
  : _type (Type::Text)
  {
    new (&_store.textval) std::string (std::move (val));
  }

  Value::Value (const char* val)
  : _type (Type::Text)
  {
    new (&_store.textval) std::string (val);
  }

  Value::Value (double val) noexcept
  : _type (Type::Real)
  {
    _store.realval = val;
  }

  Value::Value (Blob val) noexcept
  : _type (Type::Blob)
  {
    new (&_store.blobval) Blob (std::move (val));
  }

  Value::~Value () noexcept
  {
    if (_type == Type::Text)
      {
        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        _store.blobval.~vector<Blob::value_type> ();
      }
  }

  Value::Value (const Value& other) noexcept
  : _type (other._type)
  {
    switch (_type)
      {
      case Type::Null:
        break;

      case Type::Int:
        _store.intval = other._store.intval;
        break;

      case Type::Real:
        _store.realval = other._store.realval;
        break;

      case Type::Text:
        new (&_store.textval) std::string (other._store.textval);
        break;

      case Type::Blob:
        new (&_store.blobval) Blob (other._store.blobval);
        break;

      default:
        raiseErrUnexpected ("never reach"); // LCOV_EXCL_LINE
      }
  }

  Value::Value (Value&& other) noexcept
  : _type (other._type)
  {
    switch (_type)
      {
      case Type::Null:
        break;

      case Type::Int:
        _store.intval = std::move (other._store.intval);
        break;

      case Type::Real:
        _store.realval = std::move (other._store.realval);
        break;

      case Type::Text:
        new (&_store.textval) std::string (std::move (other._store.textval));
        other._store.textval.~basic_string ();
        break;

      case Type::Blob:
        new (&_store.blobval) Blob (std::move (other._store.blobval));
        other._store.blobval.~vector ();
        break;

      case Type::Variant:
        raiseErrUnexpected ("never reach"); // LCOV_EXCL_LINE
        break;                              // LCOV_EXCL_LINE
      }

    // important, set other to null so that clear does not trial to clear
    other._type = Type::Null;
  }

  Value&
  Value::operator= (const Value& other)
  {
    if (_type == Type::Text)
      {
        if (other._type == Type::Text)
          {
            _store.textval = other._store.textval;
            return *this;
          }

        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        if (other._type == Type::Blob)
          {
            _store.blobval = other._store.blobval;
            return *this;
          }
        _store.blobval.~vector<Blob::value_type> ();
      }

    _type = other._type;

    switch (_type)
      {
      case Type::Null:
        break;

      case Type::Int:
        _store.intval = other._store.intval;
        break;

      case Type::Real:
        _store.realval = other._store.realval;
        break;

      case Type::Text:
        new (&_store.textval) std::string (other._store.textval);
        break;

      case Type::Blob:
        new (&_store.blobval) Blob (other._store.blobval);
        break;

      case Type::Variant:
        raiseErrUnexpected ("never reach"); // LCOV_EXCL_LINE
        break;                              // LCOV_EXCL_LINE
      }

    return *this;
  }

  Value&
  Value::operator= (Value&& other)
  {
    if (_type == Type::Text)
      {
        if (other._type == Type::Text)
          {
            _store.textval = std::move (other._store.textval);
            other.setNull ();
            return *this;
          }

        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        if (other._type == Type::Blob)
          {
            _store.blobval = std::move (other._store.blobval);
            other.setNull ();
            return *this;
          }

        _store.blobval.~vector<Blob::value_type> ();
      }

    _type = other._type;

    switch (_type)
      {
      case Type::Null:
        break;

      case Type::Int:
        _store.intval = std::move (other._store.intval);
        break;

      case Type::Real:
        _store.realval = std::move (other._store.realval);
        break;

      case Type::Text:
        new (&_store.textval) std::string (std::move (other._store.textval));
        other._store.textval.~basic_string ();
        break;

      case Type::Blob:
        new (&_store.blobval) Blob (std::move (other._store.blobval));
        other._store.blobval.~vector ();
        break;

      case Type::Variant:
        raiseErrUnexpected ("never reach"); // LCOV_EXCL_LINE
        break;                              // LCOV_EXCL_LINE
      }

    // important, set other to null so that clear does not trial to clear
    other._type = Type::Null;

    return *this;
  }

  Value&
  Value::operator= (int val)
  {
    if (_type == Type::Text)
      {
        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        _store.blobval.~vector<Blob::value_type> ();
      }
    _store.intval = val;
    _type         = Type::Int;
    return *this;
  }

  Value&
  Value::operator= (const int64_t& val)
  {
    if (_type == Type::Text)
      {
        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        _store.blobval.~vector<Blob::value_type> ();
      }
    _store.intval = val;
    _type         = Type::Int;
    return *this;
  }

  Value&
  Value::operator= (const double& val)
  {
    if (_type == Type::Text)
      {
        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        _store.blobval.~vector<Blob::value_type> ();
      }
    _store.realval = val;
    _type          = Type::Real;
    return *this;
  }

  Value&
  Value::operator= (const std::string& val)
  {
    if (_type == Type::Text)
      {
        _store.textval = val;
        return *this;
      }
    else if (_type == Type::Blob)
      {
        _store.blobval.~vector<Blob::value_type> ();
      }

    new (&_store.textval) std::string (val);
    _type = Type::Text;
    return *this;
  }

  Value&
  Value::operator= (const Blob& val)
  {
    if (_type == Type::Text)
      {
        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        _store.blobval = val;
        return *this;
      }
    new (&_store.blobval) Blob (val);
    _type = Type::Blob;
    return *this;
  }

  Value::operator int () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();
    else if (_type == Type::Real)
      return losslessConvert1<double, int64_t> (_store.realval);
    else if (_type != Type::Int)
      throw ErrTypeMisMatch ("Implicit conversion: " + typeName (_type)
                             + " to int64_t");

    using limit = std::numeric_limits<int>;

    if (_store.intval < limit::min () || _store.intval > limit::max ())
      throw ErrOutOfRange ("Implicit conversion int64_t to int, value to big");

    return _store.intval;
  }

  Value::operator int64_t () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();
    else if (_type == Type::Real)
      return losslessConvert1<double, int64_t> (_store.realval);
    else if (_type != Type::Int)
      throw ErrTypeMisMatch ("Implicit conversion: " + typeName (_type)
                             + " to int64_t");

    return _store.intval;
  }

  Value::operator double () const
  {
    if (isNull ())
      {
        throw ErrNullValueAccess ();
      }
    else if (_type == Type::Int)
      {
        using limit = std::numeric_limits<double>;

        if (_store.intval < limit::min () || _store.intval > limit::max ())
          throw ErrOutOfRange ();

        return _store.intval;
      }
    else if (_type != Type::Real)
      {
        throw ErrTypeMisMatch (typeName (_type) + " != "
                               + typeName (Type::Real));
      }

    return _store.realval;
  }

  Value::operator const std::string& () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();
    else if (_type != Type::Text)
      throw ErrTypeMisMatch (typeName (_type) + " != "
                             + typeName (Type::Text));

    return _store.textval;
  }

  Value::operator const Blob& () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();
    else if (_type != Type::Blob)
      throw ErrTypeMisMatch (typeName (_type) + " != "
                             + typeName (Type::Blob));

    return _store.blobval;
  }

  const int64_t&
  Value::int64 () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();

    const auto wanted = Type::Int;
    if (_type != wanted)
      throw ErrTypeMisMatch (typeName (_type) + " != " + typeName (wanted));

    return _store.intval;
  }

  const double&
  Value::real () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();

    const auto wanted = Type::Real;
    if (_type != wanted)
      throw ErrTypeMisMatch (typeName (_type) + " != " + typeName (wanted));

    return _store.realval;
  }

  const std::string&
  Value::text () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();

    const auto wanted = Type::Text;
    if (_type != wanted)
      throw ErrTypeMisMatch (typeName (_type) + " != " + typeName (wanted));

    return _store.textval;
  }

  const Blob&
  Value::blob () const
  {
    if (isNull ())
      throw ErrNullValueAccess ();

    const auto wanted = Type::Blob;
    if (_type != wanted)
      throw ErrTypeMisMatch (typeName (_type) + " != " + typeName (wanted));

    return _store.blobval;
  }



  bool
  Value::operator== (const int val) const
  {
    if (_type == Type::Int)
      return _store.intval == val;
    else if (_type == Type::Real)
      return _store.realval == val;

    return false;
  }

  bool
  Value::operator== (const int64_t& val) const
  {
    if (_type == Type::Int)
      return _store.intval == val;
    else if (_type == Type::Real)
      return _store.realval == val;

    return false;
  }

  bool
  Value::operator== (const std::string& val) const
  {
    if (_type == Type::Text)
      return _store.textval == val;

    return false;
  }

  bool
  Value::operator== (const double& val) const
  {
    if (_type == Type::Real)
      return almost_equal (_store.realval, val, 2);
    else if (_type == Type::Int)
      return val == _store.intval; // good enought?

    return false;
  }

  bool
  Value::operator== (const Blob& val) const
  {
    if (_type == Type::Blob)
      return _store.blobval == val;

    return false;
  }



  std::string
  Value::ejectText ()
  {
    if (_type == Type::Null)
      throw ErrNullValueAccess ();
    else if (_type != Type::Text)
      throw ErrTypeMisMatch (typeName (_type) + " != "
                             + typeName (Type::Text));

    auto tmp = std::move (_store.textval);
    setNull ();
    return tmp;
  }

  Blob
  Value::ejectBlob ()
  {
    if (_type == Type::Null)
      throw ErrNullValueAccess ();
    else if (_type != Type::Blob)
      throw ErrTypeMisMatch (typeName (_type) + " != "
                             + typeName (Type::Blob));

    auto tmp = std::move (_store.blobval);
    setNull ();
    return tmp;
  }

  void
  Value::setNull () noexcept
  {
    if (_type == Type::Text)
      {
        _store.textval.~basic_string<std::string::value_type> ();
      }
    else if (_type == Type::Blob)
      {
        _store.blobval.~vector<Blob::value_type> ();
      }
    _type = Type::Null;
  }

  bool
  Value::isNull () const noexcept
  {
    return _type == Type::Null;
  }

  Type
  Value::getType () const noexcept
  {
    return _type;
  }

  std::ostream&
  operator<< (std::ostream& stm, const sl3::Value& v)
  {
    switch (v.getType ())
      {
      case Type::Null:
        stm << "<NULL>";
        break;

      case Type::Int:
        stm << v._store.intval;
        break;

      case Type::Real:
        stm << v._store.realval;
        break;

      case Type::Text:
        stm << v._store.textval;
        break;

      case Type::Blob:
        stm << "<BLOB>";
        break;

      default:
        stm << "unknown storage type !!"; // LCOV_EXCL_LINE
        break;                            // LCOV_EXCL_LINE
      }

    return stm;
  }

  bool
  operator== (const Value& a, const Value& b) noexcept
  {

    if (a.getType () != b.getType())
      return false ;

    bool retval = false;

    switch (a.getType ())
      {
      case Type::Null:
        retval = b.isNull ();
        break;

      case Type::Int:
//        if (b._type == Type::Int)
          retval = a._store.intval == b._store.intval;

        break;

      case Type::Real:
  //      if (b._type == Type::Real)
          retval = a._store.realval == b._store.realval;

        break;

      case Type::Text:
    //    if (b._type == Type::Text)
          retval = a._store.textval == b._store.textval;
        break;

      case Type::Blob:
      //  if (b._type == Type::Blob)
          retval = a._store.blobval == b._store.blobval;
        break;

      default:
        break; // LCOV_EXCL_LINE
      }

    return retval;
  }

  bool
  operator< (const Value& a, const Value& b) noexcept
  {
    if (b.isNull ())
      return false;

    if (a.isNull ())
      return true;

    if (a.getType () == Type::Int)
      {
        if (b.getType () == Type::Int)
          return a._store.intval < b._store.intval;

        if (b.getType () == Type::Real)
        {
          if (a._store.intval <= b._store.realval)
            return true;
          else 
            return false ; 
         }

        return true;
      }

    if (a.getType () == Type::Real)
      {
        if (b.getType () == Type::Int)
        {
          if (a._store.realval < b._store.intval)
            return true ;
          else 
            return false;
        }

        if (b.getType () == Type::Real)
          return a._store.realval < b._store.realval;

        return true;
      }

    if (a.getType () == Type::Text)
      {
        if (b.getType () == Type::Text)
          return a._store.textval < b._store.textval;

        if (b.getType () == Type::Blob)
          return true;
        else
          return false;
      }

    // TODO assert a.getType () == Type::Blob

    // this is blob
    if (b.getType () != Type::Blob)
      return false;

    // we are both bolb
    return a._store.blobval < b._store.blobval;
  }

  void
  swap (Value& a, Value& b) noexcept
  {
    auto t{std::move (a)};
    a = std::move (b);
    b = std::move (t);
  }


  bool
  weak_eq (const Value& a, const Value& b) noexcept
  {
    bool retval = false;

    switch (a.getType ())
      {
      case Type::Null:
        retval = b.isNull ();
        break;

      case Type::Int:
        if (b._type == Type::Int)
          retval = a._store.intval == b._store.intval;
        else if (b._type == Type::Real)
          retval = a._store.intval == b._store.realval;

        break;

      case Type::Real:
        if (b._type == Type::Int)
          retval = a._store.realval == b._store.intval;
        else if (b._type == Type::Real)
          retval = a._store.realval == b._store.realval;

        break;

      case Type::Text:
        if (b._type == Type::Text)
          retval = a._store.textval == b._store.textval;
        break;

      case Type::Blob:
        if (b._type == Type::Blob)
          retval = a._store.blobval == b._store.blobval;
        break;

      default:
        break; // LCOV_EXCL_LINE
      }

    return retval;
  }


  bool
  weak_lt (const Value& a, const Value& b) noexcept
  {
    if (b.isNull ())
      return false;

    if (a.isNull ())
      return true;

    if (a.getType () == Type::Int)
      {
        if (b.getType () == Type::Int)
          return a._store.intval < b._store.intval;

        if (b.getType () == Type::Real)
          return a._store.intval < b._store.realval;

        return true;
      }

    if (a.getType () == Type::Real)
      {
        if (b.getType () == Type::Int)
          return a._store.realval < b._store.intval;

        if (b.getType () == Type::Real)
          return a._store.realval < b._store.realval;

        return true;
      }

    if (a.getType () == Type::Text)
      {
        if (b.getType () == Type::Text)
          return a._store.textval < b._store.textval;

        if (b.getType () == Type::Blob)
          return true;
        else
          return false;
      }

    // TODO assert a.getType () == Type::Blob

    // this is blob
    if (b.getType () != Type::Blob)
      return false;

    // we are both bolb
    return a._store.blobval < b._store.blobval;
  }



} // ns
