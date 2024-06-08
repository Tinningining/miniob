/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2023/06/28.
//

#include <sstream>
#include <cmath>
#include "sql/parser/value.h"
#include "storage/field/field.h"
#include "common/log/log.h"
#include "common/lang/comparator.h"
#include "common/lang/string.h"

const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "floats","DATES","booleans"};

const char *attr_type_to_string(AttrType type)
{
  if (type >= UNDEFINED && type <= DATES) {
    return ATTR_TYPE_NAME[type];
  }
  return "unknown";
}
AttrType attr_type_from_string(const char *s)
{
  for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
    if (0 == strcmp(ATTR_TYPE_NAME[i], s)) {
      return (AttrType)i;
    }
  }
  return UNDEFINED;
}

Value::Value(int val)
{
  set_int(val);
}

Value::Value(float val)
{
  set_float(val);
}

Value::Value(bool val)
{
  set_boolean(val);
}

Value::Value(const char *s, int len /*= 0*/)
{
  set_string(s, len);
}

Value::Value(const char *date, int len, int flag)
{
  int intDate = -1;
  strDate_to_intDate(date,intDate);
  if(intDate == -1)
  {
    throw "illegal date";
  }
  else set_date(intDate);
}

void Value::set_data(char *data, int length)
{
  switch (attr_type_) {
    case CHARS: {
      set_string(data, length);
    } break;
    case INTS: {
      num_value_.int_value_ = *(int *)data;
      length_ = length;
    } break;
    case FLOATS: {
      num_value_.float_value_ = *(float *)data;
      length_ = length;
    } break;
    case BOOLEANS: {
      num_value_.bool_value_ = *(int *)data != 0;
      length_ = length;
    } break;
    case DATES: {
      num_value_.date_value_ = *(int *)data;
      length_ = length;
    } break;
    default: {
      LOG_WARN("unknown data type: %d", attr_type_);
    } break;
  }
}
void Value::set_int(int val)
{
  attr_type_ = INTS;
  num_value_.int_value_ = val;
  length_ = sizeof(val);
}

void Value::set_float(float val)
{
  attr_type_ = FLOATS;
  num_value_.float_value_ = val;
  length_ = sizeof(val);
}
void Value::set_boolean(bool val)
{
  attr_type_ = BOOLEANS;
  num_value_.bool_value_ = val;
  length_ = sizeof(val);
}
void Value::set_string(const char *s, int len /*= 0*/)
{
  attr_type_ = CHARS;
  if (len > 0) {
    len = strnlen(s, len);
    str_value_.assign(s, len);
  } else {
    str_value_.assign(s);
  }
  length_ = str_value_.length();
}
void Value::set_date(int val)
{
  attr_type_ = DATES;
  num_value_.date_value_ = val;
  length_ = sizeof(val);
}

void Value::set_value(const Value &value)
{
  switch (value.attr_type_) {
    case INTS: {
      set_int(value.get_int());
    } break;
    case FLOATS: {
      set_float(value.get_float());
    } break;
    case CHARS: {
      set_string(value.get_string().c_str());
    } break;
    case BOOLEANS: {
      set_boolean(value.get_boolean());
    } break;
    case DATES: {
      set_date(value.get_date());
    } break;
    case UNDEFINED: {
      ASSERT(false, "got an invalid value type");
    } break;
  }
}

const char *Value::data() const
{
  switch (attr_type_) {
    case CHARS: {
      return str_value_.c_str();
    } break;
    default: {
      return (const char *)&num_value_;
    } break;
  }
}

std::string Value::to_string() const
{
  std::stringstream os;
  switch (attr_type_) {
    case INTS: {
      os << num_value_.int_value_;
    } break;
    case FLOATS: {
      os << common::double_to_str(num_value_.float_value_);
    } break;
    case BOOLEANS: {
      os << num_value_.bool_value_;
    } break;
    case CHARS: {
      os << str_value_;
    } break;
    case DATES: {
      char strDate[11] = "";
      intDate_to_strDate(num_value_.date_value_,strDate);
      os << strDate;
    } break;
    default: {
      LOG_WARN("unsupported attr type: %d", attr_type_);
    } break;
  }
  return os.str();
}

int Value::compare(const Value &other) const
{
  if (this->attr_type_ == other.attr_type_) {
    switch (this->attr_type_) {
      case INTS: {
        return common::compare_int((void *)&this->num_value_.int_value_, (void *)&other.num_value_.int_value_);
      } break;
      case FLOATS: {
        return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other.num_value_.float_value_);
      } break;
      case CHARS: {
        return common::compare_string((void *)this->str_value_.c_str(),
            this->str_value_.length(),
            (void *)other.str_value_.c_str(),
            other.str_value_.length());
      } break;
      case BOOLEANS: {
        return common::compare_int((void *)&this->num_value_.bool_value_, (void *)&other.num_value_.bool_value_);
      } break;
      case DATES: {
        return common::compare_date((void *)&this->num_value_.date_value_, (void *)&other.num_value_.date_value_);
      } 
      default: {
        LOG_WARN("unsupported type: %d", this->attr_type_);
      }
    }
  } else if (this->attr_type_ == INTS && other.attr_type_ == FLOATS) {
    float this_data = this->num_value_.int_value_;
    return common::compare_float((void *)&this_data, (void *)&other.num_value_.float_value_);
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == INTS) {
    float other_data = other.num_value_.int_value_;
    return common::compare_float((void *)&this->num_value_.float_value_, (void *)&other_data);
  } else if (this->attr_type_ == INTS && other.attr_type_ == CHARS) {
    float this_data = this->num_value_.int_value_;
    float other_data = string_to_float(other.str_value_.c_str());
    return common::compare_float((void *)&this_data, (void *)&other_data);
  } else if (this->attr_type_ == CHARS && other.attr_type_ == INTS) {
    float other_data = other.num_value_.int_value_;
    float this_data = string_to_float(this->str_value_.c_str());
    return common::compare_float((void *)&this_data, (void *)&other_data);
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == CHARS) {
    float this_data = this->num_value_.float_value_;
    float other_data = string_to_float(other.str_value_.c_str());
    return common::compare_float((void *)&this_data, (void *)&other_data);
  } else if (this->attr_type_ == CHARS && other.attr_type_ == FLOATS) {
    float other_data = other.num_value_.float_value_;
    float this_data = string_to_float(this->str_value_.c_str());
    return common::compare_float((void *)&this_data, (void *)&other_data);
  }


  // } else if (this->attr_type_ == INTS && other.attr_type_ == CHARS) {
  //   std::string this_data = std::to_string(this->num_value_.int_value_);
  //   return common::compare_string((void *)(this_data.c_str()), this_data.length(), (void *)(other.str_value_.c_str()), other.str_value_.length());
  // } else if (this->attr_type_ == CHARS && other.attr_type_ == INTS) { 
  //   std::string other_data = std::to_string(other.num_value_.int_value_);
  //   return common::compare_string((void*)(this->str_value_.c_str()), this->str_value_.length(), (void*)(other_data.c_str()), other_data.length());
  // } else if (this->attr_type_ == FLOATS && other.attr_type_ == CHARS) {
  //   // std::string this_data = std::to_string(this->num_value_.float_value_);
  //   // return common::compare_string((void *)(this_data.c_str()), this_data.length(), (void *)(other.str_value_.c_str()), other.str_value_.length());
  //   std::string this_data = removeFloatStringEndZero(std::to_string(this->num_value_.float_value_));
  //   std::string other_data = removeFloatStringEndZero(floatString_to_String(other.str_value_));
  //   return common::compare_string((void *)(this_data.c_str()), this_data.length(), (void *)(other_data.c_str()), other_data.length());
  // } else if (this->attr_type_ == CHARS && other.attr_type_ == FLOATS) {
  //   // std::string other_data = std::to_string(other.num_value_.float_value_);
  //   // return common::compare_string((void*)(this->str_value_.c_str()), this->str_value_.length(), (void*)(other_data.c_str()), other_data.length());
  //   std::string other_data = removeFloatStringEndZero(std::to_string(other.num_value_.float_value_));
  //   std::string this_data = removeFloatStringEndZero(floatString_to_String(this->str_value_));
  //   return common::compare_string((void *)(this_data.c_str()), this_data.length(), (void *)(other_data.c_str()), other_data.length());
  // }
  LOG_WARN("not supported");
  return -1;  // TODO return rc?
}

int Value::get_int() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return (int)(std::stol(str_value_));
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0;
      }
    }
    case INTS: {
      return num_value_.int_value_;
    }
    case FLOATS: {
      return (int)(num_value_.float_value_);
    }
    case BOOLEANS: {
      return (int)(num_value_.bool_value_);
    }
    // case DATES: {
    //   return num_value_.date_value_;
    // }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

float Value::get_float() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return std::stof(str_value_);
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to float. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0.0;
      }
    } break;
    case INTS: {
      return float(num_value_.int_value_);
    } break;
    case FLOATS: {
      return num_value_.float_value_;
    } break;
    case BOOLEANS: {
      return float(num_value_.bool_value_);
    } break;
    // case DATES: {
    //   return float(num_value_.date_value_);
    // } break;
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

std::string Value::get_string() const
{
  return this->to_string();
}

bool Value::get_boolean() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        float val = std::stof(str_value_);
        if (val >= EPSILON || val <= -EPSILON) {
          return true;
        }

        int int_val = std::stol(str_value_);
        if (int_val != 0) {
          return true;
        }

        return !str_value_.empty();
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to float or integer. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return !str_value_.empty();
      }
    } break;
    case INTS: {
      return num_value_.int_value_ != 0;
    } break;
    case FLOATS: {
      float val = num_value_.float_value_;
      return val >= EPSILON || val <= -EPSILON;
    } break;
    case BOOLEANS: {
      return num_value_.bool_value_;
    } break;
    // case DATES: {
    //   return num_value_.date_value_ != 0;
    // }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return false;
    }
  }
  return false;
}

int Value::get_date() const
{
  switch (attr_type_) {
    case CHARS: {
      try {
        return (int)(std::stol(str_value_));
      } catch (std::exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", str_value_.c_str(), ex.what());
        return 0;
      }
    }
    case INTS: {
      return num_value_.int_value_;
    }
    case FLOATS: {
      return (int)(num_value_.float_value_);
    }
    case BOOLEANS: {
      return (int)(num_value_.bool_value_);
    }
    case DATES: {
      return num_value_.date_value_;
    }
    default: {
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

bool is_leap_year(int year)
{
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

bool check_date(int y, int m, int d)
{
  static int mon[] = {0, 31, 28, 31, 30, 31, 30, 31,31,30,31,30,31};
  bool leap = is_leap_year(y);
  return y > 0
  && (m > 0) && (m <= 12)
  && (d > 0) && (d <=((m == 2 && leap) ? 1 : 0) + mon[m]);
}

void strDate_to_intDate(const char* strDate,int& intDate)
{
  int y,m,d;
  int ret = sscanf(strDate,"%d-%d-%d",&y,&m,&d);
  if(ret != 3 || !check_date(y,m,d))
  {
    return;
  }
  intDate = 10000*y + 100*m + d;
} 

void intDate_to_strDate(const int intDate, char strDate[11])
{
  snprintf(strDate,11,"%04d-%02d-%02d", intDate/10000, (intDate % 10000)/100, intDate % 100);
}

float string_to_float(const std::string& str){
  float result = 0.0;
  float integer = 0.0;
  float fraction = 0.0;
  int exponent = 0;
  bool isDot = false;
  if(str.empty() || str[0] < '0' || str[0] > '9'){
    return 0;
  }
  for(int i = 0;i < str.length();i++){

    if(str[i] < '0' || str[i] > '9'){
      if(str[i] == '.' && !isDot){
        isDot = true;
        continue;
      }
      else 
        return result;
    }

    if(!isDot){
      integer = integer * 10 + (str[i] - '0');
    }
    else{
      fraction = fraction * 10 + (str[i] - '0');
      exponent--;
    }

    result = integer + fraction * pow(10, exponent);
  }

  return result;

}

// std::string floatString_to_String(std::string floatString){
//   std::string ret = "";
//   int i = 0;
//   bool dot_flag = false;
//   while(floatString[i] >= '0' && floatString[i] <= '9'){
//     ret += floatString[i];
//     i++;
//     if (!dot_flag && floatString[i] == '.') {
//       i++;
//       ret += '.';
//       dot_flag = true;
//     }
//   }
//   return ret == "" ? floatString : ret;
// }

// std::string removeFloatStringEndZero(std::string str){
//   std::string ret = "";
//   for(int i = str.size() - 1; i >= 0; i--)
//     if(str[i] == '0')
//       continue;
//     else
//       ret = str[i] + ret;
//   if(ret == "")
//     ret = "0";
//   return ret;
// }

