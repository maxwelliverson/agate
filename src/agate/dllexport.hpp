//
// Created by Maxwell on 2024-02-05.
//

#ifndef AGATE_SUPPORT_DLLEXPORT_HPP
#define AGATE_SUPPORT_DLLEXPORT_HPP

#if defined(agate_support_EXPORTS)
# define AGT_dllexport __declspec(dllexport)
#else
# define AGT_dllexport __declspec(dllimport)
#endif

#endif //AGATE_SUPPORT_DLLEXPORT_HPP
