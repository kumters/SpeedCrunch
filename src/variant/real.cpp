/* real.cpp: real numbers */
/*
    Copyright (C) 2008 Wolf Lammen.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License , or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to:

      The Free Software Foundation, Inc.
      59 Temple Place, Suite 330
      Boston, MA 02111-1307 USA.


    You may contact the author by:
       e-mail:  ookami1 <at> gmx <dot> de
       mail:  Wolf Lammen
              Oertzweg 45
              22307 Hamburg
              Germany

*************************************************************************/

#include "variant/real.hxx"
#include "math/floatconfig.h"
#include "math/floatconvert.h"

enum { less = 1, equal = 2, greater = 4 };

static floatstruct NaNVal;
static int longrealPrec;

static int _cvtMode(LongReal::FmtMode mode)
{
  switch (mode)
  {
    case LongReal::FixPoint: return IO_MODE_FIXPOINT;
    case LongReal::Engineering: return IO_MODE_ENG;
    case LongReal::Complement2: return IO_MODE_COMPLEMENT;
    default: return IO_MODE_SCIENTIFIC;
  }
}

static LongReal::Sign _cvtSign(signed char sign)
{
  switch (sign)
  {
    case IO_SIGN_COMPLEMENT: return LongReal::Compl2;
    case IO_SIGN_MINUS     : return LongReal::Minus;
    case IO_SIGN_PLUS      : return LongReal::Plus;
    default                : return LongReal::None;
  }
}

static signed char _cvtSign(LongReal::Sign sign)
{
  switch (sign)
  {
    case LongReal::Compl2: return IO_SIGN_COMPLEMENT;
    case LongReal::Minus : return IO_SIGN_MINUS;
    case LongReal::Plus  : return IO_SIGN_PLUS;
    default              : return IO_SIGN_NONE;
  }
}

static char _mod(floatnum dest, cfloatnum dividend, cfloatnum modulo)
{
  enum { maxdivloops = 250 };
  int save = float_setprecision(int(maxdivloops));
  floatstruct dummy;
  float_create(&dummy);
  char result = float_divmod(&dummy, dest, dividend, modulo, INTQUOT);
  float_free(&dummy);
  float_setprecision(save);
  return result;
}

static char _idiv(floatnum dest, cfloatnum dividend, cfloatnum modulo)
{
  int save = float_setprecision(DECPRECISION);
  floatstruct dummy;
  float_create(&dummy);
  char result = float_divmod(dest, &dummy, dividend, modulo, INTQUOT);
  float_free(&dummy);
  float_setprecision(save);
  return result;
}

void LongReal::initClass()
{
  precision(precDefault);
  float_create(&NaNVal);
  registerConstructor(create, vLongReal);
}

LongReal::LongReal()
  : refcount(1)
{
  float_create(&val);
}

LongReal::~LongReal()
{
  float_free(&val);
}

VariantData* LongReal::create()
{
  return new LongReal;
}

void LongReal::release()
{
  if (--refcount <= 0)
    delete this;
}

VariantData* LongReal::clone() const
{
  ++refcount;
  return const_cast<LongReal*>(this);
}

cfloatnum LongReal::NaN()
{
  return &NaNVal;
}

bool LongReal::move(floatnum x)
{
  if (refcount != 1)
    return false;
  float_move(&val, x);
  return true;
}

bool LongReal::assign(const char* str)
{
  if (refcount != 1)
    return false;
  float_setscientific(&val, str, NULLTERMINATED);
  return !float_isnan(&val);
}

Variant LongReal::call2(const Variant& other, Fct2 fct, bool swap) const
{
  if (type() != other.type())
    return NotImplemented;
  floatstruct result;
  float_create(&result);
  (swap? fct(&result, other, *this, evalPrec())
       : fct(&result, *this, other, evalPrec()))
    && float_round(&result, &result, workPrec(), TONEAREST);
  return Variant(&result, float_geterror());
}

Variant LongReal::call2ND(const Variant& other, Fct2ND fct, bool swap) const
{
  if (type() != other.type())
    return NotImplemented;
  floatstruct result;
  float_create(&result);
  if (swap)
    fct(&result, other, *this);
  else
    fct(&result, *this, other);
  return Variant(&result, float_geterror());
}

Variant LongReal::callCmp(const Variant& other, char mask) const
{
  if (type() != other.type())
    return NotImplemented;
  signed char cmp = float_cmp(*this, other);
  if (cmp == UNORDERED)
    return NoOperand;
  if (cmp < 0 && (mask & less) != 0)
    return true;
  if (cmp > 0 && (mask & greater) != 0)
    return true;
  if (cmp == 0 && (mask & equal) != 0)
    return true;
  return false;
}

Variant LongReal::operator+() const
{
  if (float_isnan(&val))
    return NoOperand;
  return this;
}

Variant LongReal::operator-() const
{
  floatstruct result;
  float_create(&result);
  float_copy(&result, &val, EXACT);
  float_neg(&result);
  return Variant(&result, float_geterror());
}

Variant LongReal::operator+(const Variant& other) const
{
  return call2(other, float_add);
}

Variant LongReal::operator-(const Variant& other) const
{
  return call2(other, float_sub);
}

Variant LongReal::operator*(const Variant& other) const
{
  return call2(other, float_add);
}

Variant LongReal::operator/(const Variant& other) const
{
  return call2(other, float_add);
}

Variant LongReal::operator%(const Variant& other) const
{
  return call2ND(other, _mod);
}

Variant LongReal::idiv(const Variant& other) const
{
  return call2ND(other, _idiv);
}

Variant LongReal::operator==(const Variant& other) const
{
  return callCmp(other, equal);
}

Variant LongReal::operator!=(const Variant& other) const
{
  return callCmp(other, less | greater);
}

Variant LongReal::operator>(const Variant& other) const
{
  return callCmp(other, greater);
}

Variant LongReal::operator>=(const Variant& other) const
{
  return callCmp(other, greater | equal);
}

Variant LongReal::operator<(const Variant& other) const
{
  return callCmp(other, less);
}

Variant LongReal::operator<=(const Variant& other) const
{
  return callCmp(other, less | equal);
}

Variant LongReal::swapSub(const Variant& other) const
{
  return call2(other, float_sub, true);
}

Variant LongReal::swapDiv(const Variant& other) const
{
  return call2(other, float_div, true);
}

Variant LongReal::swapMod(const Variant& other) const
{
  return call2ND(other, _mod, true);
}

Variant LongReal::swapIdiv(const Variant& other) const
{
  return call2ND(other, _idiv, true);
}

int LongReal::precision(int newprec)
{
  int result = longrealPrec;
  if (newprec == 0 || newprec > DECPRECISION)
    newprec = DECPRECISION;
  if (newprec > 0)
    longrealPrec = newprec;
  return result;
}

bool LongReal::isNaN() const
{
  return float_isnan(&val);
}

bool LongReal::isZero() const
{
  return float_iszero(&val);
}

int LongReal::evalPrec()
{
  return longrealPrec + 5;
}

int LongReal::workPrec()
{
  return longrealPrec + 3;
}

LongReal::operator QByteArray() const
{
  char buffer[DECPRECISION+30];
  float_getscientific(buffer, sizeof(buffer), &val);
  return buffer;
}

LongReal::BasicIO LongReal::convert(int prec, FmtMode mode,
                   char base, char scalebase) const
{
  t_otokens tokens;
  floatstruct workcopy;
  BasicIO result;
  char intpart[BINPRECISION+5];
  char fracpart[BINPRECISION+5];
  char scale[BITS_IN_EXP+2];
  t_buffer scaleBuf;

  tokens.intpart.buf = intpart;
  tokens.intpart.sz = sizeof(intpart);
  tokens.fracpart.buf = fracpart;
  tokens.fracpart.sz = sizeof(fracpart);
  float_create(&workcopy);
  float_copy(&workcopy, &val, evalPrec());
  scale[0] = 0;
  result.signScale = LongReal::None;
  result.error = float_out(&tokens, &workcopy, prec,
                           base, scalebase, _cvtMode(mode));
  if (result.error == Success)
    switch (mode)
    {
      case LongReal::Scientific:
      case LongReal::Engineering:
        scaleBuf.sz = sizeof(scale);
        scaleBuf.buf = scale;
        if (tokens.exp > 0)
          result.signScale = LongReal::Plus;
        else if (tokens.exp < 0)
          result.signScale = LongReal::Minus;
        result.error = exp2str(&scaleBuf, tokens.exp, scalebase);
      default: ;
    }
  if (result.error == Success)
  {
    result.baseSignificand = base;
    result.baseScale = scalebase;
    result.signSignificand = _cvtSign(tokens.sign);
    result.intpart = QString::fromAscii(tokens.intpart.buf);
    result.fracpart = QString::fromAscii(tokens.fracpart.buf);
    result.scale = QString::fromAscii(scale);
  }
  return result;
}

Variant LongReal::convert(const BasicIO& io)
{
  t_itokens tokens;
  QByteArray intpart = io.intpart.toUtf8();
  QByteArray fracpart = io.fracpart.toUtf8();
  tokens.intpart = intpart.data();
  tokens.fracpart = intpart.data();
  tokens.exp = 0;
  tokens.expbase = IO_BASE_NAN;
  tokens.expsign = IO_SIGN_NONE;
  tokens.sign = _cvtSign(io.signSignificand);
  tokens.base = io.baseSignificand;
  tokens.maxdigits = evalPrec();
  if (!io.scale.isEmpty())
  {
    QByteArray scale = io.scale.toUtf8();
    tokens.exp = scale.data();
    tokens.expbase = io.baseScale;
    tokens.expsign = io.signScale;
  }
  floatstruct val;
  float_create(&val);
  Error e = float_in(&val, &tokens);
  return Variant(&val, e);
}

void RealFormat::setMode(LongReal::FmtMode m, int dgt, char b, char sb, int prec)
{
  mode = m;
  base = b;
  scalebase = sb;
  if (prec <= 0 || prec > DECPRECISION)
    precision = DECPRECISION;
  else
    precision = prec;
  int maxdgt;
  switch (b)
  {
    case  2: maxdgt = precision * 2136;
    case  8: maxdgt = precision * 712;
    case 16: maxdgt = precision * 534;
    default: maxdgt = precision * 643;
  }
  maxdgt /= 643;
  if (dgt <= 0 || dgt > maxdgt)
    digits = maxdgt;
  else
    digits = dgt;
}

QString RealFormat::getSignificandPrefix()
{
  return QString();
}

QString RealFormat::getSignificandSuffix()
{
  return QString();
}

QString RealFormat::getScalePrefix()
{
  return QString();
}

QString RealFormat::getScaleSuffix()
{
  return QString();
}

QString RealFormat::formatNaN()
{
  return "NaN";
}

QString RealFormat::formatZero()
{
  return "0";
}

QString RealFormat::formatInt(const QString& seq, LongReal::Sign sign)
{
  return QString();
}

QString RealFormat::formatFrac(const QString& seq)
{
  return QString();
}

QString RealFormat::formatScale(const QString& seq, LongReal::Sign sign)
{
  return QString();
}

QString RealFormat::format(const VariantData& val)
{
  const LongReal* vr = dynamic_cast<const LongReal*>(&val);
  if (!vr)
    return QString();
  if (vr->isNaN())
    return formatNaN();
  if (vr->isZero())
    return formatZero();
  LongReal::BasicIO basicIO = vr->convert(precision, mode, base, scalebase);
  if (basicIO.error != Success)
    return QString();
  return getSignificandPrefix();
}
