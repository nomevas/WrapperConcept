package com.nomenas.wrapperconcept.project;

/**
 * Created by naum.puroski on 13/04/2017.
 */

public class MultipleInheritance extends SimpleClass {
    private BaseClass baseClass = new BaseClass();

    public MultipleInheritance() {create();};

    public int baseClassMethod() {
        return baseClass.baseClassMethod();
    }

    protected native void create();
    protected native void delete();
}
