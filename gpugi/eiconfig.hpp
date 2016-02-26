#pragma once

/// The configuration system of epsilon works as follows:
///  All files expect to include the config.hpp. The repository only contains
///  a defaultconfig.hpp. This should be copied and renamed accordingly. Thus,
///  you can use epsilon as submodule in other repositories. Since the new
///  (renamed) config file is not part of this repository you can change it as
///  you want.
///
/// To change an option include or uncomment lines to customize the described
/// properties.


/// \brief All types from elementarytypes.hpp are without namespace.
/// \details The default is 'enabled'.
#define EI_GLOBAL_ELEMENTARIES

/// \brief Using this option changes the namespace ei:: to ε::.
/// \details Do not use this if the symbol epsilon above is not shown correctly,
///    or enable utf8 in all your files.
///
///    The default is 'disabled'.
#define EI_USE_UNICODE_NAMES


/// \brief Enable a bunch of debug assertions in the vector code.
/// \details As usual the assertions are not contained in release mode. This
///    macro lets you further disable the assertions in debug mode to speed
///    up the application.
///
///    Level 0: Deactivate all assertions. Not recommended.
///    Level 1: Use assertions where errors are likely. Default option.
///    Level 2: Maximum. Permanent tests everywhere.
#define EI_ASSERTION_LEVEL		2
