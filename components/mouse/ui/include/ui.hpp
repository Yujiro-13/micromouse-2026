#ifndef UI_HPP
#define UI_HPP

#include "base_func.hpp"
#include "adachi.hpp"
#include "files.hpp"

struct UI : Micromouse
{
    virtual void main_task() = 0;
    virtual void ref_by_motion(Adachi &_adachi) = 0;
};

#endif // UI_HPP