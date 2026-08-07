/**
 * @file command.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_COMMAND_H
#define INCLUDE_AUTOINPUT_COMMAND_H
#pragma once


namespace autoinput
{
    struct ICommandData
    {
        virtual ~ICommandData() = default;
        [[nodiscard]] virtual bool validate() const = 0;
    };
}

#endif // INCLUDE_AUTOINPUT_COMMAND_H
