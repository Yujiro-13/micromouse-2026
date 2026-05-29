#ifndef UI_HPP
#define UI_HPP

#include "Base_func.hpp"
#include "../Motion/Adachi.hpp"
#include "files.hpp"

#define _interface struct

_interface UI : Micromouse
{
    virtual void main_task() = 0;
    virtual void ref_by_motion(Adachi &_adachi) = 0;
};

#endif // UI_HPP