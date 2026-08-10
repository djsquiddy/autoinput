/**
 * @file command.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_INPUT_COMMAND_H
#define INCLUDE_AUTOINPUT_INPUT_COMMAND_H
#pragma once


namespace autoinput
{
    struct ICommandData
    {
        virtual ~ICommandData() = default;
        [[nodiscard]] virtual bool validate() const = 0;
    };
}

#endif // INCLUDE_AUTOINPUT_INPUT_COMMAND_H
