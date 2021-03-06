/*
 * Copyright (C) 2015 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include "qemu/osdep.h"

#include "qapi/error.h"
#include "qom/object.h"
#include "qemu/module.h"
#include "qemu/option.h"
#include "qemu/config-file.h"
#include "qom/object_interfaces.h"
#include "qemu/option_int.h"
#include "hacking/hacking.h"


#define TYPE_DUMMY "qemu-dummy"

typedef struct DummyObject DummyObject;
typedef struct DummyObjectClass DummyObjectClass;

#define DUMMY_OBJECT(obj)                               \
    OBJECT_CHECK(DummyObject, (obj), TYPE_DUMMY)

typedef enum DummyAnimal DummyAnimal;

enum DummyAnimal {
    DUMMY_FROG,
    DUMMY_ALLIGATOR,
    DUMMY_PLATYPUS,

    DUMMY_LAST,
};

const QEnumLookup dummy_animal_map = {
    .array = (const char *const[]) {
        [DUMMY_FROG] = "frog",
        [DUMMY_ALLIGATOR] = "alligator",
        [DUMMY_PLATYPUS] = "platypus",
    },
    .size = DUMMY_LAST
};

struct DummyObject {
    Object parent_obj;

    bool bv;
    DummyAnimal av;
    char *sv;
};

struct DummyObjectClass {
    ObjectClass parent_class;
};


static void dummy_set_bv(Object *obj,
                         bool value,
                         Error **errp)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    dobj->bv = value;
}

static bool dummy_get_bv(Object *obj,
                         Error **errp)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    return dobj->bv;
}


static void dummy_set_av(Object *obj,
                         int value,
                         Error **errp)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    dobj->av = value;
}

static int dummy_get_av(Object *obj,
                        Error **errp)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    return dobj->av;
}


static void dummy_set_sv(Object *obj,
                         const char *value,
                         Error **errp)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    g_free(dobj->sv);
    dobj->sv = g_strdup(value);
}

static char *dummy_get_sv(Object *obj,
                          Error **errp)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    return g_strdup(dobj->sv);
}


static void dummy_init(Object *obj)
{
    Error *err = NULL;

    object_property_add_bool(obj, "bv",
                             dummy_get_bv,
                             dummy_set_bv,
                             &err);
    error_free_or_abort(&err);
}


static void dummy_class_init(ObjectClass *cls, void *data)
{
    object_class_property_add_bool(cls, "bv",
                                   dummy_get_bv,
                                   dummy_set_bv,
                                   NULL);
    object_class_property_add_str(cls, "sv",
                                  dummy_get_sv,
                                  dummy_set_sv,
                                  NULL);
    object_class_property_add_enum(cls, "av",
                                   "DummyAnimal",
                                   &dummy_animal_map,
                                   dummy_get_av,
                                   dummy_set_av,
                                   NULL);
}


static void dummy_finalize(Object *obj)
{
    DummyObject *dobj = DUMMY_OBJECT(obj);

    g_free(dobj->sv);
}


static const TypeInfo dummy_info = {
    .name          = TYPE_DUMMY,
    .parent        = TYPE_OBJECT,
    .instance_size = sizeof(DummyObject),
    .instance_init = dummy_init,
    .instance_finalize = dummy_finalize,
    .class_size = sizeof(DummyObjectClass),
    .class_init = dummy_class_init,
#ifdef CONFIG_HACKING
    .class_name = "DummyObjectClass",
#endif
    .interfaces = (InterfaceInfo[]) {
        { TYPE_USER_CREATABLE },
        { }
    }
};


/*
 * The following 3 object classes are used to
 * simulate the kind of relationships seen in
 * qdev, which result in complex object
 * property destruction ordering.
 *
 * DummyDev has a 'bus' child to a DummyBus
 * DummyBus has a 'backend' child to a DummyBackend
 * DummyDev has a 'backend' link to DummyBackend
 *
 * When DummyDev is finalized, it unparents the
 * DummyBackend, which unparents the DummyDev
 * which deletes the 'backend' link from DummyDev
 * to DummyBackend. This illustrates that the
 * object_property_del_all() method needs to
 * cope with the list of properties being changed
 * while it iterates over them.
 */
typedef struct DummyDev DummyDev;
typedef struct DummyDevClass DummyDevClass;
typedef struct DummyBus DummyBus;
typedef struct DummyBusClass DummyBusClass;
typedef struct DummyBackend DummyBackend;
typedef struct DummyBackendClass DummyBackendClass;

#define TYPE_DUMMY_DEV "qemu-dummy-dev"
#define TYPE_DUMMY_BUS "qemu-dummy-bus"
#define TYPE_DUMMY_BACKEND "qemu-dummy-backend"

#define DUMMY_DEV(obj)                               \
    OBJECT_CHECK(DummyDev, (obj), TYPE_DUMMY_DEV)
#define DUMMY_BUS(obj)                               \
    OBJECT_CHECK(DummyBus, (obj), TYPE_DUMMY_BUS)
#define DUMMY_BACKEND(obj)                               \
    OBJECT_CHECK(DummyBackend, (obj), TYPE_DUMMY_BACKEND)

struct DummyDev {
    Object parent_obj;

    DummyBus *bus;
};

struct DummyDevClass {
    ObjectClass parent_class;
};

struct DummyBus {
    Object parent_obj;

    DummyBackend *backend;
};

struct DummyBusClass {
    ObjectClass parent_class;
};

struct DummyBackend {
    Object parent_obj;
};

struct DummyBackendClass {
    ObjectClass parent_class;
};


static void dummy_dev_finalize(Object *obj)
{
    DummyDev *dev = DUMMY_DEV(obj);

    object_unref(OBJECT(dev->bus));
}

static void dummy_dev_init(Object *obj)
{
    DummyDev *dev = DUMMY_DEV(obj);
    DummyBus *bus = DUMMY_BUS(object_new(TYPE_DUMMY_BUS));
    DummyBackend *backend = DUMMY_BACKEND(object_new(TYPE_DUMMY_BACKEND));

    object_property_add_child(obj, "bus", OBJECT(bus), NULL);
    dev->bus = bus;
    object_property_add_child(OBJECT(bus), "backend", OBJECT(backend), NULL);
    bus->backend = backend;

    object_property_add_link(obj, "backend", TYPE_DUMMY_BACKEND,
                             (Object **)&bus->backend, NULL, 0, NULL);
}

static void dummy_dev_unparent(Object *obj)
{
    DummyDev *dev = DUMMY_DEV(obj);
    object_unparent(OBJECT(dev->bus));
}

static void dummy_dev_class_init(ObjectClass *klass, void *opaque)
{
    klass->unparent = dummy_dev_unparent;
}


static void dummy_bus_finalize(Object *obj)
{
    DummyBus *bus = DUMMY_BUS(obj);

    object_unref(OBJECT(bus->backend));
}

static void dummy_bus_init(Object *obj)
{
}

static void dummy_bus_unparent(Object *obj)
{
    DummyBus *bus = DUMMY_BUS(obj);
    object_property_del(obj->parent, "backend", NULL);
    object_unparent(OBJECT(bus->backend));
}

static void dummy_bus_class_init(ObjectClass *klass, void *opaque)
{
    klass->unparent = dummy_bus_unparent;
}

static void dummy_backend_init(Object *obj)
{
}


static const TypeInfo dummy_dev_info = {
    .name          = TYPE_DUMMY_DEV,
    .parent        = TYPE_OBJECT,
    .instance_size = sizeof(DummyDev),
    .instance_init = dummy_dev_init,
    .instance_finalize = dummy_dev_finalize,
    .class_size = sizeof(DummyDevClass),
    .class_init = dummy_dev_class_init,
};

static const TypeInfo dummy_bus_info = {
    .name          = TYPE_DUMMY_BUS,
    .parent        = TYPE_OBJECT,
    .instance_size = sizeof(DummyBus),
    .instance_init = dummy_bus_init,
    .instance_finalize = dummy_bus_finalize,
    .class_size = sizeof(DummyBusClass),
    .class_init = dummy_bus_class_init,
};

static const TypeInfo dummy_backend_info = {
    .name          = TYPE_DUMMY_BACKEND,
    .parent        = TYPE_OBJECT,
    .instance_size = sizeof(DummyBackend),
    .instance_init = dummy_backend_init,
    .class_size = sizeof(DummyBackendClass),
};

static QemuOptsList qemu_object_opts = {
    .name = "object",
    .implied_opt_name = "qom-type",	/* a implied option: qom-type=TYPE_DUMMY */
    .head = QTAILQ_HEAD_INITIALIZER(qemu_object_opts.head),
    .desc = {
#ifdef CONFIG_HACKING
	    /* add some desc for some option */
        {
            .name = "qom-type",
            .type = QEMU_OPT_STRING,
            .help = "type of object to create",
	    .def_value_str = "extra_value",
        },
        {
            .name = "bv",
            .type = QEMU_OPT_STRING /*QEMU_OPT_BOOL*/,	/* two different levels of QemuOpt and Object Property.
					   when set BOOL type here, we must use on/off in QemuOpt level.
					   but we can use yes/no on OptVisitor.
					   see difference in qemu_opt_parse() and opts_type_bool() */
            .help = "bool value",
        },
        {
            .name = "av",
            .type = QEMU_OPT_STRING,	/* is enum, but for QemuOpt level, we use string */
            .help = "enum number",
        },
        {
            .name = "sv",
            .type = QEMU_OPT_STRING,
            .help = "some string",
        },
#endif
        { /* end of list */ }
    },
};


static void test_dummy_createv(void)
{
    Error *err = NULL;
    Object *parent = object_get_objects_root();
    DummyObject *dobj = DUMMY_OBJECT(
        object_new_with_props(TYPE_DUMMY,
                              parent,
                              "dummy0",
                              &err,
                              "bv", "yes",
                              "sv", "Hiss hiss hiss",
                              "av", "platypus",
                              NULL));

    g_assert(err == NULL);
    g_assert_cmpstr(dobj->sv, ==, "Hiss hiss hiss");
    g_assert(dobj->bv == true);
    g_assert(dobj->av == DUMMY_PLATYPUS);

    g_assert(object_resolve_path_component(parent, "dummy0")
             == OBJECT(dobj));

#ifdef CONFIG_HACKING
    g_print("parent canonical path: %s\n", object_get_canonical_path(parent));
    ghash_table_dump("props of parent Object:", parent->properties, print_properties_table);

    Object *o = OBJECT(dobj);
    g_print("object canonical path: %s\n", object_get_canonical_path(o));
    ghash_table_dump("props of Object:", o->properties, print_properties_table);

    ObjectClass *oc = OBJECT_CLASS(object_get_class(dobj));
    ghash_table_dump("props of ObjectClass:", oc->properties, print_properties_table);
#endif

    object_unparent(OBJECT(dobj));
}


static Object *new_helper(Error **errp,
                          Object *parent,
                          ...)
{
    va_list vargs;
    Object *obj;

    va_start(vargs, parent);
    obj = object_new_with_propv(TYPE_DUMMY,
                                parent,
                                "dummy0",
                                errp,
                                vargs);
    va_end(vargs);
    return obj;
}

static void test_dummy_createlist(void)
{
    Error *err = NULL;
    Object *parent = object_get_objects_root();
    DummyObject *dobj = DUMMY_OBJECT(
        new_helper(&err,
                   parent,
                   "bv", "yes",
                   "sv", "Hiss hiss hiss",
                   "av", "platypus",
                   NULL));

    g_assert(err == NULL);
    g_assert_cmpstr(dobj->sv, ==, "Hiss hiss hiss");
    g_assert(dobj->bv == true);
    g_assert(dobj->av == DUMMY_PLATYPUS);

    g_assert(object_resolve_path_component(parent, "dummy0")
             == OBJECT(dobj));

    object_unparent(OBJECT(dobj));
}

static void test_dummy_createcmdl(void)
{
    QemuOpts *opts;
    DummyObject *dobj;
    Error *err = NULL;
    const char *params = TYPE_DUMMY \
                         ",id=dev0," \
                         "bv=yes,sv=Hiss hiss hiss,av=platypus";

    qemu_add_opts(&qemu_object_opts);
    opts = qemu_opts_parse(&qemu_object_opts, params, true, &err);
    g_assert(err == NULL);
    g_assert(opts);

#ifdef CONFIG_HACKING
    /* QemuOptsList(with desc)
     * -> QemuOpts(identified by id)
     * -> QemuOpt(name&value) */
    qemu_opts_print_help(&qemu_object_opts, true);

    QemuOpts *tempopts;
    QTAILQ_FOREACH(tempopts, &qemu_object_opts.head, next) {
	    qemu_opts_print(tempopts, "|");
    }
#endif

    dobj = DUMMY_OBJECT(user_creatable_add_opts(opts, &err));
    g_assert(err == NULL);
    g_assert(dobj);
    g_assert_cmpstr(dobj->sv, ==, "Hiss hiss hiss");
    g_assert(dobj->bv == true);
    g_assert(dobj->av == DUMMY_PLATYPUS);

#ifdef CONFIG_HACKING
    Object *parent = object_get_objects_root();
    g_print("\nparent canonical path: %s\n", object_get_canonical_path(parent));
    ghash_table_dump("props of parent Object:", parent->properties, print_properties_table);

    Object *o = OBJECT(dobj);
    g_print("object canonical path: %s\n", object_get_canonical_path(o));
    ghash_table_dump("props of Object:", o->properties, print_properties_table);

    ObjectClass *oc = OBJECT_CLASS(object_get_class(dobj));
    ghash_table_dump("props of ObjectClass:", oc->properties, print_properties_table);
#endif

    user_creatable_del("dev0", &err);
    g_assert(err == NULL);
    error_free(err);

    object_unref(OBJECT(dobj));

    /*
     * cmdline-parsing via qemu_opts_parse() results in a QemuOpts entry
     * corresponding to the Object's ID to be added to the QemuOptsList
     * for objects. To avoid having this entry conflict with future
     * Objects using the same ID (which can happen in cases where
     * qemu_opts_parse() is used to parse the object params, such as
     * with hmp_object_add() at the time of this comment), we need to
     * check for this in user_creatable_del() and remove the QemuOpts if
     * it is present.
     *
     * The below check ensures this works as expected.
     */
    g_assert_null(qemu_opts_find(&qemu_object_opts, "dev0"));
}

static void test_dummy_badenum(void)
{
    Error *err = NULL;
    Object *parent = object_get_objects_root();
    Object *dobj =
        object_new_with_props(TYPE_DUMMY,
                              parent,
                              "dummy0",
                              &err,
                              "bv", "yes",
                              "sv", "Hiss hiss hiss",
                              "av", "yeti",
                              NULL);

    g_assert(dobj == NULL);
    g_assert(err != NULL);
    g_assert_cmpstr(error_get_pretty(err), ==,
                    "Invalid parameter 'yeti'");

    g_assert(object_resolve_path_component(parent, "dummy0")
             == NULL);

    error_free(err);
}


static void test_dummy_getenum(void)
{
    Error *err = NULL;
    int val;
    Object *parent = object_get_objects_root();
    DummyObject *dobj = DUMMY_OBJECT(
        object_new_with_props(TYPE_DUMMY,
                         parent,
                         "dummy0",
                         &err,
                         "av", "platypus",
                         NULL));

    g_assert(err == NULL);
    g_assert(dobj->av == DUMMY_PLATYPUS);

    val = object_property_get_enum(OBJECT(dobj),
                                   "av",
                                   "DummyAnimal",
                                   &err);
    g_assert(err == NULL);
    g_assert(val == DUMMY_PLATYPUS);

    /* A bad enum type name */
    val = object_property_get_enum(OBJECT(dobj),
                                   "av",
                                   "BadAnimal",
                                   &err);
    g_assert(err != NULL);
    error_free(err);
    err = NULL;

    /* A non-enum property name */
    val = object_property_get_enum(OBJECT(dobj),
                                   "iv",
                                   "DummyAnimal",
                                   &err);
    g_assert(err != NULL);
    error_free(err);

    object_unparent(OBJECT(dobj));
}


/* we may use object_property_iter_next() in our ghash_table_dump(),
 * but if we use this, we'll get an extra propperty named "type", which
 * is added in object_class_init(). This makes sense from OO's view. */
static void test_dummy_prop_iterator(ObjectPropertyIterator *iter)
{
    bool seenbv = false, seensv = false, seenav = false, seentype = false;
    ObjectProperty *prop;

    while ((prop = object_property_iter_next(iter))) {
        if (!seenbv && g_str_equal(prop->name, "bv")) {
            seenbv = true;
        } else if (!seensv && g_str_equal(prop->name, "sv")) {
            seensv = true;
        } else if (!seenav && g_str_equal(prop->name, "av")) {
            seenav = true;
        } else if (!seentype && g_str_equal(prop->name, "type")) {
            /* This prop comes from the base Object class */
            seentype = true;
        } else {
            g_printerr("Found prop '%s'\n", prop->name);
            g_assert_not_reached();
        }
    }
    g_assert(seenbv);
    g_assert(seenav);
    g_assert(seensv);
    g_assert(seentype);
}

static void test_dummy_iterator(void)
{
    Object *parent = object_get_objects_root();
    DummyObject *dobj = DUMMY_OBJECT(
        object_new_with_props(TYPE_DUMMY,
                              parent,
                              "dummy0",
                              &error_abort,
                              "bv", "yes",
                              "sv", "Hiss hiss hiss",
                              "av", "platypus",
                              NULL));
    ObjectPropertyIterator iter;

    object_property_iter_init(&iter, OBJECT(dobj));
    test_dummy_prop_iterator(&iter);
    object_unparent(OBJECT(dobj));
}

static void test_dummy_class_iterator(void)
{
    ObjectPropertyIterator iter;
    ObjectClass *klass = object_class_by_name(TYPE_DUMMY);

    object_class_property_iter_init(&iter, klass);
    test_dummy_prop_iterator(&iter);
}

static void test_dummy_delchild(void)
{
    Object *parent = object_get_objects_root();
    DummyDev *dev = DUMMY_DEV(
        object_new_with_props(TYPE_DUMMY_DEV,
                              parent,
                              "dev0",
                              &error_abort,
                              NULL));

    object_unparent(OBJECT(dev));
}

static void test_qom_partial_path(void)
{
    Object *root  = object_get_objects_root();
    Object *cont1 = container_get(root, "/cont1");
    Object *obj1  = object_new(TYPE_DUMMY);
    Object *obj2a = object_new(TYPE_DUMMY);
    Object *obj2b = object_new(TYPE_DUMMY);
    bool ambiguous;

    /* Objects created:
     * /cont1
     * /cont1/obj1
     * /cont1/obj2 (obj2a)
     * /obj2 (obj2b)
     */
    object_property_add_child(cont1, "obj1", obj1, &error_abort);
    object_unref(obj1);
    object_property_add_child(cont1, "obj2", obj2a, &error_abort);
    object_unref(obj2a);
    object_property_add_child(root,  "obj2", obj2b, &error_abort);
    object_unref(obj2b);

    ambiguous = false;
    g_assert(!object_resolve_path_type("", TYPE_DUMMY, &ambiguous));
    g_assert(ambiguous);
    g_assert(!object_resolve_path_type("", TYPE_DUMMY, NULL));

    ambiguous = false;
    g_assert(!object_resolve_path("obj2", &ambiguous));
    g_assert(ambiguous);
    g_assert(!object_resolve_path("obj2", NULL));

    ambiguous = false;
    g_assert(object_resolve_path("obj1", &ambiguous) == obj1);
    g_assert(!ambiguous);
    g_assert(object_resolve_path("obj1", NULL) == obj1);

    object_unparent(obj2b);
    object_unparent(cont1);

#ifdef CONFIG_HACKING
    /* all containers will be created on demand in container_get() */
    Object *child_cont = container_get(root, "/pcont/cont");
    Object *obj_in_p = object_new(TYPE_DUMMY);
    Object *obj_in_c = object_new(TYPE_DUMMY);

    object_property_add_child(child_cont, "childobj", obj_in_c, &error_abort);
    object_unref(obj_in_c);

    g_print("child container canonical path: %s\n", object_get_canonical_path(child_cont));
    g_print("obj_in_child_container canonical path: %s\n", object_get_canonical_path(obj_in_c));

    ambiguous = false;
    Object *parent_cont = object_resolve_path("pcont", &ambiguous);
    g_assert(!ambiguous);
    g_assert(parent_cont);
    object_property_add_child(parent_cont, "parentdobj", obj_in_p, &error_abort);
    object_unref(obj_in_p);

    g_print("parent containercanonical path: %s\n", object_get_canonical_path(parent_cont));
    g_print("obj_in_parent_container canonical path: %s\n", object_get_canonical_path(obj_in_p));

    object_unparent(obj_in_c);
    object_unparent(obj_in_p);
    object_unparent(child_cont);
    object_unparent(parent_cont);
#endif
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    module_call_init(MODULE_INIT_QOM);
    type_register_static(&dummy_info);
    type_register_static(&dummy_dev_info);
    type_register_static(&dummy_bus_info);
    type_register_static(&dummy_backend_info);

    g_test_add_func("/qom/proplist/createlist", test_dummy_createlist);
    g_test_add_func("/qom/proplist/createv", test_dummy_createv);
    g_test_add_func("/qom/proplist/createcmdline", test_dummy_createcmdl);
    g_test_add_func("/qom/proplist/badenum", test_dummy_badenum);
    g_test_add_func("/qom/proplist/getenum", test_dummy_getenum);
    g_test_add_func("/qom/proplist/iterator", test_dummy_iterator);
    g_test_add_func("/qom/proplist/class_iterator", test_dummy_class_iterator);
    g_test_add_func("/qom/proplist/delchild", test_dummy_delchild);
    g_test_add_func("/qom/resolve/partial", test_qom_partial_path);

    return g_test_run();
}
