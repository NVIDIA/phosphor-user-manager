project(
    'phosphor-user-manager',
    'cpp',
    version: '0.1',
    meson_version: '>=1.1.1',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
        'buildtype=debugoptimized',
    ],
)

if get_option('root_user_mgmt').allowed()
<<<<<<< HEAD
    add_project_arguments('-DENABLE_ROOT_USER_MGMT', language: 'cpp')
endif
if get_option('ipmi').allowed()
    add_project_arguments('-DENABLE_IPMI', language: 'cpp')
||||||| 34e6ccd
    add_project_arguments('-DENABLE_ROOT_USER_MGMT', language:'cpp')
=======
    add_project_arguments('-DENABLE_ROOT_USER_MGMT', language: 'cpp')
>>>>>>> origin/master
endif
conf_data = configuration_data()

<<<<<<< HEAD
conf_data.set_quoted(
    'USER_MANAGER_BUSNAME',
    'xyz.openbmc_project.User.Manager',
    description: 'The DBus busname to own.',
)

conf_data.set(
    'CLASS_VERSION',
    1,
    description: 'Class version to register with Cereal.',
)

conf_data.set_quoted(
    'LDAP_CONFIG_FILE',
    '/etc/nslcd.conf',
    description: 'Path of LDAP configuration file.',
)

conf_data.set_quoted(
    'TLS_CACERT_PATH',
    '/etc/ssl/certs/authority',
    description: 'Path of LDAP server CA certificate.',
)

conf_data.set_quoted(
    'TLS_CERT_FILE',
    '/etc/nslcd/certs/cert.pem',
    description: 'Path of LDAP client certificate.',
)

conf_data.set_quoted(
    'LDAP_CONFIG_ROOT',
    '/xyz/openbmc_project/user/ldap',
    description: 'LDAP configuration root.',
)
||||||| 34e6ccd
conf_data.set_quoted('USER_MANAGER_BUSNAME', 'xyz.openbmc_project.User.Manager',
                      description : 'The DBus busname to own.')
=======
conf_data.set_quoted(
    'USER_MANAGER_BUSNAME',
    'xyz.openbmc_project.User.Manager',
    description: 'The DBus busname to own.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set_quoted(
    'LDAP_CONFIG_DBUS_OBJ_PATH',
    '/xyz/openbmc_project/user/ldap/config',
    description: 'D-Bus path of LDAP config object.',
)
||||||| 34e6ccd
conf_data.set('CLASS_VERSION', 1,
                      description : 'Class version to register with Cereal.')
=======
conf_data.set(
    'CLASS_VERSION',
    1,
    description: 'Class version to register with Cereal.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set_quoted(
    'LDAP_CONFIG_BUSNAME',
    'xyz.openbmc_project.Ldap.Config',
    description: 'D-Bus busname of LDAP config service.',
)
||||||| 34e6ccd
conf_data.set_quoted('LDAP_CONFIG_FILE', '/etc/nslcd.conf',
                      description : 'Path of LDAP configuration file.')
=======
conf_data.set_quoted(
    'LDAP_CONFIG_FILE',
    '/etc/nslcd.conf',
    description: 'Path of LDAP configuration file.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set_quoted(
    'LDAP_CONF_PERSIST_PATH',
    '/var/lib/phosphor-ldap-conf',
    description: 'path of directory having persisted LDAP configuration enabled property.',
)
||||||| 34e6ccd
conf_data.set_quoted('TLS_CACERT_PATH', '/etc/ssl/certs/authority',
                      description : 'Path of LDAP server CA certificate.')
=======
conf_data.set_quoted(
    'TLS_CACERT_PATH',
    '/etc/ssl/certs/authority',
    description: 'Path of LDAP server CA certificate.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set(
    'MIN_PASSWORD_LENGTH',
    get_option('MIN_PASSWORD_LENGTH'),
    description: 'Minimum password length',
)
||||||| 34e6ccd
conf_data.set_quoted('TLS_CERT_FILE', '/etc/nslcd/certs/cert.pem',
                      description : 'Path of LDAP client certificate.')
=======
conf_data.set_quoted(
    'TLS_CERT_FILE',
    '/etc/nslcd/certs/cert.pem',
    description: 'Path of LDAP client certificate.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set(
    'MIN_PASSWORD_LENGTH',
    get_option('MIN_PASSWORD_LENGTH'),
    description: 'Minimum password length',
)
||||||| 34e6ccd
conf_data.set_quoted('LDAP_CONFIG_ROOT', '/xyz/openbmc_project/user/ldap',
                      description : 'LDAP configuration root.')
=======
conf_data.set_quoted(
    'LDAP_CONFIG_ROOT',
    '/xyz/openbmc_project/user/ldap',
    description: 'LDAP configuration root.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set(
    'MAX_PASSWORD_LENGTH',
    get_option('MAX_PASSWORD_LENGTH'),
    description: 'Maximum password length',
)
||||||| 34e6ccd
conf_data.set_quoted('LDAP_CONFIG_DBUS_OBJ_PATH', '/xyz/openbmc_project/user/ldap/config',
                      description : 'D-Bus path of LDAP config object.')
=======
conf_data.set_quoted(
    'LDAP_CONFIG_DBUS_OBJ_PATH',
    '/xyz/openbmc_project/user/ldap/config',
    description: 'D-Bus path of LDAP config object.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set(
    'ACCOUNT_UNLOCK_TIMEOUT',
    get_option('ACCOUNT_UNLOCK_TIMEOUT'),
    description: 'Account unlock timeout in seconds',
)
||||||| 34e6ccd
conf_data.set_quoted('LDAP_CONFIG_BUSNAME', 'xyz.openbmc_project.Ldap.Config',
                      description : 'D-Bus busname of LDAP config service.')
=======
conf_data.set_quoted(
    'LDAP_CONFIG_BUSNAME',
    'xyz.openbmc_project.Ldap.Config',
    description: 'D-Bus busname of LDAP config service.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set(
    'MAX_FAILED_LOGIN_ATTEMPTS',
    get_option('MAX_FAILED_LOGIN_ATTEMPTS'),
    description: 'Maximum failed login attempts',
)
||||||| 34e6ccd
conf_data.set_quoted('LDAP_CONF_PERSIST_PATH', '/var/lib/phosphor-ldap-conf',
                      description : 'path of directory having persisted LDAP configuration enabled property.')
=======
conf_data.set_quoted(
    'LDAP_CONF_PERSIST_PATH',
    '/var/lib/phosphor-ldap-conf',
    description: 'path of directory having persisted LDAP configuration enabled property.',
)
>>>>>>> origin/master

<<<<<<< HEAD
conf_data.set(
    'CREATE_USER_HOME_FOLDER',
    get_option('CREATE_USER_HOME_FOLDER'),
    description: 'Whether to create home folder for new users',
)

conf_data.set(
    'SKIP_USERS_IN_PROTECTED_GROUP',
    get_option('SKIP_USERS_IN_PROTECTED_GROUP'),
    description: 'Whether to skip users in protected groups',
)

policy_update_adoption_type_as_string = get_option('POLICY_UPDATE_ADOPTION_TYPE')
if policy_update_adoption_type_as_string == 'default'
    policy_update_adoption_type_as_num = '1'
elif policy_update_adoption_type_as_string == 'conditional'
    policy_update_adoption_type_as_num = '2'
elif policy_update_adoption_type_as_string == 'universal'
    policy_update_adoption_type_as_num = '3'
else
    error('Invalid policy update adoption type')
endif
conf_data.set(
    'POLICY_UPDATE_ADOPTION_TYPE',
    policy_update_adoption_type_as_num,
    description: 'Policy update adoption type',
)

conf_data.set(
    'POLICY_UPDATE_PASSWORD_EXPIRY',
    get_option('POLICY_UPDATE_PASSWORD_EXPIRY').to_string(),
    description: 'Password expiry policy update setting',
)

conf_data.set(
    'POLICY_UPDATE_PASSWORD_EXPIRY_EXCLUSION_LIST',
    '{@0@}'.format(
        ','.join(get_option('POLICY_UPDATE_PASSWORD_EXPIRY_EXCLUSION_LIST')),
    ),
    description: 'List of users excluded from password expiry policy updates',
)

root_users = get_option('ROOT_PRIVILEGE_USER_LIST')
quoted_users = []

foreach u : root_users
    quoted_users += '"@0@"'.format(u)
endforeach

conf_data.set(
    'ROOT_PRIVILEGE_USER_LIST',
    '{' + ', '.join(quoted_users) + '}',
    description: 'Quoted string list for C++ initializer list',
)

conf_header = configure_file(output: 'config.h', configuration: conf_data)
||||||| 34e6ccd
conf_header = configure_file(output: 'config.h',
    configuration: conf_data)
=======
conf_data.set(
    'MAX_PASSWORD_LENGTH',
    get_option('MAX_PASSWORD_LENGTH'),
    description: 'Maximum password length',
)
>>>>>>> origin/master

conf_header = configure_file(output: 'config.h', configuration: conf_data)

cpp = meson.get_compiler('cpp')

ldap_dep = cpp.find_library('ldap', required: get_option('ldap'))
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
pam_dep = dependency('pam')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
systemd_dep = dependency('systemd')

# Get Cereal dependency.
cereal_dep = dependency('cereal', required: false)
has_cereal = cpp.has_header_symbol(
    'cereal/cereal.hpp',
    'cereal::specialize',
    dependencies: cereal_dep,
    required: false,
)
if not has_cereal
    cereal_opts = import('cmake').subproject_options()
    cereal_opts.add_cmake_defines(
        {'BUILD_TESTS': 'OFF', 'SKIP_PERFORMANCE_COMPARISON': 'ON'},
    )
    cereal_proj = import('cmake').subproject(
        'cereal',
        options: cereal_opts,
        required: false,
    )
    assert(cereal_proj.found(), 'cereal is required')
    cereal_dep = cereal_proj.dependency('cereal')
endif
user_manager_src = ['mainapp.cpp', 'user_mgr.cpp', 'users.cpp']

<<<<<<< HEAD
user_manager_src = ['mainapp.cpp', 'user_mgr.cpp', 'users.cpp']
||||||| 34e6ccd
user_manager_src = [
    'mainapp.cpp',
    'user_mgr.cpp',
    'users.cpp'
]
=======
>>>>>>> origin/master

user_manager_deps = [
<<<<<<< HEAD
    boost_dep,
    sdbusplus_dep,
    phosphor_logging_dep,
    phosphor_dbus_interfaces_dep,
||||||| 34e6ccd
     boost_dep,
     sdbusplus_dep,
     phosphor_logging_dep,
     phosphor_dbus_interfaces_dep
=======
    nlohmann_json_dep,
    pam_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
>>>>>>> origin/master
]

cpp_flags_loc = []

user_manager_lib = static_library(
    'phosphor-user-manager',
    ['user_mgr.cpp', 'users.cpp'],
    dependencies: user_manager_deps,
    cpp_args: cpp_flags_loc,
)

user_manager_dep = declare_dependency(
    link_with: user_manager_lib,
    dependencies: user_manager_deps,
)

executable(
    'phosphor-user-manager',
    'mainapp.cpp',
    dependencies: user_manager_dep,
<<<<<<< HEAD
    link_args: ['-lcrypt'],
    cpp_args: [
        '-DBOOST_ALL_NO_LIB',
        '-DBOOST_SYSTEM_NO_DEPRECATED',
        '-DBOOST_ERROR_CODE_HEADER_ONLY',
    ],
||||||| 34e6ccd
    link_args: ['-lcrypt' ],
    cpp_args: ['-DBOOST_ALL_NO_LIB', '-DBOOST_SYSTEM_NO_DEPRECATED', '-DBOOST_ERROR_CODE_HEADER_ONLY'],
=======
    link_args: ['-lcrypt'],
>>>>>>> origin/master
    install: true,
)


<<<<<<< HEAD
systemd_system_unit_dir = systemd_dep.get_variable('systemdsystemunitdir')
||||||| 34e6ccd
systemd_system_unit_dir = systemd_dep.get_variable(
    'systemdsystemunitdir'
)
=======
systemd_system_unit_dir = systemd_dep.get_variable('systemd_system_unit_dir')
>>>>>>> origin/master

install_data(
    'phosphor-nslcd-cert-config.conf',
    install_dir: get_option('datadir') / 'dbus-1' / 'system.d',
)

install_data(
    'nslcd',
    install_dir: get_option('datadir') / 'phosphor-certificate-manager',
)

install_data('mfa_pam', install_dir: '/etc/pam.d/')

# Figure out how to use install_symlink to install symlink to a file of another
# recipe
#install_symlink(
#      'phosphor-certificate-manager@nslcd.service',
#      install_dir: systemd_system_unit_dir / 'multi-user.target.wants',
#      pointing_to: systemd_system_unit_dir / 'phosphor-certificate-manager@.service',
#  )
meson.add_install_script(
    'sh',
    '-c',
    'mkdir -p $(dirname $DESTDIR/@0@/@1@)'.format(
        systemd_system_unit_dir,
        'multi-user.target.wants/phosphor-certificate-manager@nslcd.service',
    ),
)
meson.add_install_script(
    'sh',
    '-c',
    'ln -s @0@ $DESTDIR/@1@/@2@'.format(
        '../phosphor-certificate-manager@.service',
        systemd_system_unit_dir,
        'multi-user.target.wants/phosphor-certificate-manager@nslcd.service',
    ),
)

if get_option('ldap').allowed() and ldap_dep.found()
    subdir('phosphor-ldap-config')
endif

if get_option('tests').allowed()
    subdir('test')
endif
