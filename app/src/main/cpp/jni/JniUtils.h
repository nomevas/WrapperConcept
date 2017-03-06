//
// Created by Naum Puroski on 05/03/2017.
//

#ifndef WRAPPERCONCEPT_JNIUTILS_H
#define WRAPPERCONCEPT_JNIUTILS_H

#include <functional>

template <typename T>
struct from{};

template <typename T>
struct to{};

template <typename From, typename To>
struct to_base {
    using Type = To;
    to_base<From, To>(JNIEnv*, From value) : _value(value) {}
    Type value() const {return _value;}
    operator Type() const {return _value;}
    Type _value;
};

template <typename From, typename To>
struct from_base {
    using Type = To;
    from_base<From, To>(JNIEnv*, Type value) : _value(value){}
    Type value() const { return _value;}
    operator Type() const {return _value;}
    Type _value;
};

template <>
struct to<int> : to_base<jint, int> { using to_base<jint, int>::to_base; };
template<>
struct from<int> : from_base<int, jint>{ using from_base<int, jint>::from_base; };

template <>
struct to<long> : to_base<jlong, long> { using to_base<jlong, long>::to_base; };
template<>
struct from<long> : from_base<long, jlong>{ using from_base<long, jlong>::from_base; };

template<>
struct to<bool> : to_base<jboolean, bool> { using to_base<jboolean, bool>::to_base; };
template<>
struct from<bool> : from_base<bool, jboolean>{ using from_base<bool, jboolean>::from_base; };

template<>
struct to<void*> {
    using Type = void*; \
    to<Type>(JNIEnv*, jlong value) : _value(reinterpret_cast<void*>(value)) {} \
    Type value() const {return _value;} \
    operator Type() const {return _value;} \
    Type _value;
};
template<>
struct from<void*> {
    using Type = jlong;
    template <typename T>
    from<long>(JNIEnv*, T* ptr) : _value(reinterpret_cast<long>(ptr)){};
    Type value() const { return _value;}
    Type _value;
};

template<>
struct to<std::string> {
    using Type = std::string;
    to<Type>(JNIEnv* env, jstring value) {
        const char *text = env->GetStringUTFChars(value, 0);
        _value = text;
        env->ReleaseStringUTFChars(value, text);
    }
    Type value() const {
        return _value;
    }
    operator Type() const {return _value;}
    Type _value;
};

template<>
struct from<std::string> {
    using Type = jstring;
    from<std::string>(JNIEnv* env, const std::string& value) : _value(env->NewStringUTF(value.c_str())) {}
    Type value() const { return _value;}
    operator Type() const {return _value;}
    Type _value;
};

template <typename... Args>
void call_java_void_method(JNIEnv *env, jobject instance, char* method, char* signature, Args... args) {
    jmethodID methodId = env->GetMethodID(env->GetObjectClass(instance), method, signature);
    env->CallVoidMethod(instance, methodId, args.value()...);
}

template <typename... Args>
long call_java_long_method(JNIEnv *env, jobject instance, char* method, char* signature, Args... args) {
    jmethodID methodId = env->GetMethodID(env->GetObjectClass(instance), method, signature);
    return to<long>(env, env->CallLongMethod(instance, methodId, args.value()...));
}

template <typename T>
T* get_reference(JNIEnv *env, jobject instance) {
    return reinterpret_cast<T*>(call_java_long_method(env, instance, "getReference", "()J"));
}

template <typename T>
void set_reference(JNIEnv *env, jobject instance, T* obj, bool takeOwnership) {
    call_java_void_method(env, instance, "setReference", "(JZ)V", from<void*>(env, obj), from<bool>(env, takeOwnership));
}

template <typename T>
void delete_referenced_object(JNIEnv *env, jobject instance) {
    T* obj = reinterpret_cast<T*>(call_java_long_method(env, instance, "getReference", "()J"));
    if (obj != nullptr) {
        delete obj;
        obj = nullptr;
        call_java_void_method(env, instance, "setReference", "(J)V", from<void*>(env, obj));
    }
};

template <typename Class, typename Method, typename... Args>
auto call(JNIEnv *env, jobject instance, Method method, Args... args) -> decltype(std::bind(method, instance, std::forward<Args>(args)...)()) {
    Class* obj = get_reference<Class>(env, instance);
    if (obj != nullptr) {
        return std::bind(method, obj, std::forward<Args>(args)...)();
    }
};

#endif //WRAPPERCONCEPT_JNIUTILS_H
