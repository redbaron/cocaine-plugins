//
// Copyright (C) 2011-2012 Rim Zaidullin <creator@bash.org.ru>
//
// Licensed under the BSD 2-Clause License (the "License");
// you may not use this file except in compliance with the License.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <EXTERN.h>
#include <perl.h>

#include <cocaine/context.hpp>
#include <cocaine/logging.hpp>
#include <cocaine/manifest.hpp>

#include <cocaine/api/sandbox.hpp>

#include <sstream>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);

EXTERN_C void xs_init(pTHX) {
    char* file = (char*)__FILE__;
    dXSUB_SYS;

    // DynaLoader is a special case
    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

namespace cocaine {
namespace driver {

class perl_t: public api::sandbox_t {
public:
    typedef api::sandbox_t category_type;

public:
    perl_t(context_t& context,
           const std::string& name,
           const Json::Value& args,
           const std::string& spool):
        category_type(context, name, args, spool),
        m_log(context.log(
            cocaine::format("app/%1%", name)
        ))
    {
        int argc = 0;
        char** argv = NULL;
        char** env = NULL;
        PERL_SYS_INIT3(&argc, &argv, &env);

        my_perl = perl_alloc();
        perl_construct(my_perl);

        if(!args.isObject()) {
            throw configuration_error_t("malformed manifest");
        }

        Json::Value script_file = args["script-file"];
        if (!script_file.isString()) {
            throw configuration_error_t("malformed manifest, expected args script-file to be a name of the perl script to run");
        }

        boost::filesystem::path source(spool);
        source/=script_file.asString();

        if(boost::filesystem::is_directory(source)) {
            throw configuration_error_t("malformed manifest, expected path to perl script, got a directory.");
        }

        std::string source_dir;
        #if BOOST_FILESYSTEM_VERSION == 3
            source_dir = source.parent_path().string();
        #else
            source_dir = source.branch_path().string();
        #endif

        boost::filesystem::ifstream input(source);

        if(!input) {
            throw configuration_error_t("unable to open '%s'", source.string());
        }

        const char* embedding[] = {"", (char*)source.string().c_str(), "-I", (char*)source_dir.c_str()};
        perl_parse(my_perl, xs_init, 4, (char**)embedding, NULL);
        PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
        COCAINE_LOG_INFO(m_log, "%s", "running interpreter...");
        perl_run(my_perl);
    }

    ~perl_t() {
        perl_destruct(my_perl);
        perl_free(my_perl);
        PERL_SYS_TERM();
    }

    virtual void invoke(const std::string& event, api::io_t& io) {
        COCAINE_LOG_INFO(m_log, "%s", std::string("invoking event ") + event + "...");
        std::string input;

        // Try to pull in the request w/o blocking.
        std::string request = io.read(0);

        if (!request.empty()) {
           input = std::string((const char*)request.data(), request.size());
        }

        PERL_SET_CONTEXT(my_perl);

        std::string result;
        const char* input_value_buff = NULL;

        // init stack pointer
        dSP;
        ENTER;
        SAVETMPS;

        // remember stack pointer
        PUSHMARK(SP);

        int call_flags = G_EVAL | G_SCALAR | G_NOARGS;

        // if there's an argument - push it on the stack
        if (!input.empty()) {
            call_flags = G_EVAL | G_SCALAR;
            input_value_buff = input.c_str();

            XPUSHs(sv_2mortal(newSVpv(input_value_buff, 0)));
            PUTBACK;
        }

        // call function
        int ret_vals_count = call_pv(event.c_str(), call_flags);

        // refresh stack pointer
        SPAGAIN;

        // get error (if any)
        if (SvTRUE(ERRSV)) {
            STRLEN n_a;

            std::string error_msg = "perl eval error: ";
            error_msg += SvPV(ERRSV, n_a);
            throw unrecoverable_error_t(error_msg);
        }

        // pop returned value of the stack
        if (ret_vals_count > 0) {
            char* str_ptr = savepv(POPp);

            if (str_ptr) {
                result = std::string(str_ptr);
            }
        }

        // clean-up
        PUTBACK;
        FREETMPS;
        LEAVE;

        // invoke callback with resulting data
        if (!result.empty()) {
            io.write(result.data(), result.size());
        }
    }

private:
    const logging::logger_t& log() const {
        return *m_log;
    }

private:
    std::shared_ptr<logging::logger_t> m_log;
    PerlInterpreter* my_perl;
};

extern "C" {
    void initialize(api::repository_t& repository) {
        repository.insert<perl_t>("perl");
    }
}

}}
