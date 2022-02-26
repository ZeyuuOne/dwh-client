#pragma once
#include "exception"

class ConfigNotValidException : public std::exception{
  const char * what () const throw (){
    return "Config not valid.";
  }
};