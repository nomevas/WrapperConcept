//
// Created by Naum Puroski on 05/03/2017.
//

#ifndef WRAPPERCONCEPT_JNIUTILS_H
#define WRAPPERCONCEPT_JNIUTILS_H

#include <functional>
#include <unordered_map>

#include "Proxy.h"
#include "Converter.h"

namespace wrapper_core {
    template<typename JNIApi, typename... Args>
    auto call_java_method(JNIApi api, jobject instance, const char *method, const char *signature, Args... args)
        -> decltype((static_cast<JNIEnv*>(nullptr)->*api)(nullptr, nullptr, args.value()...)){
        auto env = JNIEnvFactory::Create();
        jmethodID methodId = env->GetMethodID(env->GetObjectClass(instance), method, signature);
        return (env.get()->*api)(instance, methodId, args.value()...);
    }

    template <typename T>
    Proxy<T>* get_proxy(jobject instance) {
        return reinterpret_cast<Proxy<T>*>(call_java_method(&JNIEnv::CallLongMethod, instance, "getReference", "()J"));
    }

    template<typename T>
    T *get_reference(jobject instance) {
        auto proxy = get_proxy<T>(instance);
        return proxy ? proxy->subject() : nullptr;
    }

    template<typename T>
    void delete_referenced_object(jobject instance) {
        auto proxy = get_proxy<T>(instance);
        if (proxy != nullptr) {
            delete proxy;
            proxy = nullptr;
            call_java_method(&JNIEnv::CallVoidMethod, instance, "setReference", "(J)V", from<void *>(proxy));
        }
    };

    template<typename T>
    void set_reference(jobject instance, T *obj, bool takeOwnership) {
        delete_referenced_object<T>(instance);
        call_java_method(&JNIEnv::CallVoidMethod, instance, "setReference", "(J)V",
                              from<void*>(new Proxy<T>(obj, takeOwnership)));
    }

    template<typename T>
    void set_reference(jobject instance, std::shared_ptr<T> obj) {
        delete_referenced_object<T>(instance);
        call_java_method(&JNIEnv::CallVoidMethod, instance, "setReference", "(J)V",
                              from<void*>(new Proxy<T>(obj)));
    }

    template<typename T>
    void set_reference(jobject instance, std::unique_ptr<T> obj) {
        delete_referenced_object<T>(instance);
        call_java_method(&JNIEnv::CallVoidMethod, instance, "setReference", "(J)V",
                              from<void*>(new Proxy<T>(std::move(obj))));
    }

    template<typename Class, typename Method, typename... Args>
    auto call(jobject instance, Method method, Args... args) -> decltype(std::bind(method, get_proxy<Class>(instance)->subject(),
                                                                                   std::forward<Args>(
                                                                                           args)...)()) {
        auto proxy = get_proxy<Class>(instance);
        return std::bind(method, proxy->subject(), std::forward<Args>(args)...)();
    };

    template<typename Class, typename Method, typename... Args>
    auto call(Method method, Args... args) -> decltype(std::bind(method, std::forward<Args>(args)...)()) {
        return std::bind(method, std::forward<Args>(args)...)();
    };

    template<typename Class, typename Method, typename... Args>
    jobject
    call_and_cache(const std::string &cache_key, jobject instance, Method method, Args... args) {
        jobject return_value = nullptr;

        auto env = JNIEnvFactory::Create();
        Proxy<Class> *proxy = get_proxy<Class>(instance);
        if (proxy != nullptr && env) {
            return_value = proxy->get_cache_item(cache_key);
            if (return_value == nullptr) {
                return_value = from<decltype(std::bind(method, proxy->subject(),
                                                       std::forward<Args>(args)...)())>(
                        std::bind(method, proxy->subject(), std::forward<Args>(args)...)());
                proxy->add_cache_item(cache_key, env->NewGlobalRef(return_value));
            }
        }

        return return_value;
    };
}

#define DEFINE_WRAPPER(CLASS) \
extern template struct wrapper_core::ClassInfo<CLASS>; \
template<> \
struct to<CLASS> : wrapper_core::to_base_object<CLASS> { \
    using wrapper_core::to_base_object<CLASS>::to_base_object; \
}; \
template<> \
struct from<CLASS> : wrapper_core::from_base_object<CLASS> { \
    using wrapper_core::from_base_object<CLASS>::from_base_object; \
};

#define DEFINE_ENUM(ENUM) \
extern template struct wrapper_core::ClassInfo<ENUM>; \
template<> \
struct to<ENUM> : wrapper_core::to_base_enum<ENUM> { \
    using wrapper_core::to_base_enum<ENUM>::to_base_enum; \
}; \
template<> \
struct from<ENUM> : wrapper_core::from_base_enum<ENUM> { \
    using wrapper_core::from_base_enum<ENUM>::from_base_enum; \
};

#endif //WRAPPERCONCEPT_JNIUTILS_H
