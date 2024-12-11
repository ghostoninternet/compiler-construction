#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/******************* Global Variables ******************************/
static SymTab* symbolTable = NULL;
static Type* globalIntType = NULL;
static Type* globalCharType = NULL;

/******************* Error Handling ******************************/
static void* allocateMemory(size_t size) {
    void* memory = malloc(size);
    if (!memory) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    return memory;
}

static void validateName(const char* name) {
    if (!name || strlen(name) >= MAX_IDENT_LEN) {
        fprintf(stderr, "Invalid identifier name\n");
        exit(1);
    }
}

/******************* Type Management ******************************/
Type* makeIntType(void) {
    Type* type = allocateMemory(sizeof(Type));
    type->typeClass = TP_INT;
    type->arraySize = 0;
    type->elementType = NULL;
    return type;
}

Type* makeCharType(void) {
    Type* type = allocateMemory(sizeof(Type));
    type->typeClass = TP_CHAR;
    type->arraySize = 0;
    type->elementType = NULL;
    return type;
}

Type* makeArrayType(int arraySize, Type* elementType) {
    if (arraySize <= 0 || !elementType) {
        fprintf(stderr, "Invalid array parameters\n");
        return NULL;
    }
    Type* type = allocateMemory(sizeof(Type));
    type->typeClass = TP_ARRAY;
    type->arraySize = arraySize;
    type->elementType = elementType;
    return type;
}


/******************* Constant Management ******************************/
ConstantValue* makeIntConstant(int value) {
    ConstantValue* constant = allocateMemory(sizeof(ConstantValue));
    constant->type = TP_INT;
    constant->intValue = value;
    return constant;
}

ConstantValue* makeCharConstant(char value) {
    ConstantValue* constant = allocateMemory(sizeof(ConstantValue));
    constant->type = TP_CHAR;
    constant->charValue = value;
    return constant;
}

ConstantValue* duplicateConstantValue(ConstantValue* v) {
  if (v == NULL) return NULL;
  ConstantValue* newVal = (ConstantValue*) malloc(sizeof(ConstantValue));
  newVal->type = v->type;
  if (v->type == TP_INT)
    newVal->intValue = v->intValue;
  else if (v->type == TP_CHAR)
    newVal->charValue = v->charValue;
  return newVal;
}

/******************* Object Creation ******************************/
static Object* createBaseObject(const char* name, enum ObjectKind kind) {
    validateName(name);
    Object* obj = allocateMemory(sizeof(Object));
    strcpy(obj->name, name);
    obj->kind = kind;
    return obj;
}

Object* createProgramObject(char* name) {
    Object* prog = createBaseObject(name, OBJ_PROGRAM);
    prog->progAttrs = allocateMemory(sizeof(ProgramAttributes));
    prog->progAttrs->scope = createScope(prog, NULL);
    symbolTable->program = prog;
    return prog;
}

Object* createVariableObject(char* name) {
    Object* var = createBaseObject(name, OBJ_VARIABLE);
    var->varAttrs = allocateMemory(sizeof(VariableAttributes));
    var->varAttrs->type = NULL;
    var->varAttrs->scope = symbolTable->currentScope;
    return var;
}

Object* createConstantObject(char* name) {
    Object* obj = createBaseObject(name, OBJ_CONSTANT);
    obj->constAttrs = allocateMemory(sizeof(ConstantAttributes));
    obj->constAttrs->value = NULL;
    return obj;
}

Object* createTypeObject(char* name) {
    Object* obj = createBaseObject(name, OBJ_TYPE);
    obj->typeAttrs = allocateMemory(sizeof(TypeAttributes));
    obj->typeAttrs->actualType = NULL;
    return obj;
}

Object* createFunctionObject(char* name) {
    Object* obj = createBaseObject(name, OBJ_FUNCTION);
    obj->funcAttrs = allocateMemory(sizeof(FunctionAttributes));
    obj->funcAttrs->paramList = NULL;
    obj->funcAttrs->returnType = NULL;
    obj->funcAttrs->scope = createScope(obj, symbolTable->currentScope);
    return obj;
}

Object* createProcedureObject(char* name) {
    Object* obj = createBaseObject(name, OBJ_PROCEDURE);
    obj->procAttrs = allocateMemory(sizeof(ProcedureAttributes));
    obj->procAttrs->paramList = NULL;
    obj->procAttrs->scope = createScope(obj, symbolTable->currentScope);
    return obj;
}

Object* createParameterObject(char* name, enum ParamKind kind, Object* owner) {
    Object* obj = createBaseObject(name, OBJ_PARAMETER);
    obj->paramAttrs = allocateMemory(sizeof(ParameterAttributes));
    obj->paramAttrs->kind = kind;
    obj->paramAttrs->type = NULL;
    obj->paramAttrs->function = owner;
    return obj;
}

/******************* Scope Management ******************************/
Scope* createScope(Object* owner, Scope* outer) {
    Scope* scope = allocateMemory(sizeof(Scope));
    scope->objList = NULL;
    scope->owner = owner;
    scope->outer = outer;
    return scope;
}

void enterBlock(Scope* scope) {
    if (!scope) return;
    symbolTable->currentScope = scope;
}

void exitBlock(void) {
    if (!symbolTable->currentScope->outer) return;
    symbolTable->currentScope = symbolTable->currentScope->outer;
}

/******************* Utility Functions ******************************/
void addObject(ObjectNode** objList, Object* obj) {
    ObjectNode* node = allocateMemory(sizeof(ObjectNode));
    node->object = obj;
    node->next = NULL;
    
    if (*objList == NULL) {
        *objList = node;
    } else {
        ObjectNode* current = *objList;
        while (current->next != NULL)
            current = current->next;
        current->next = node;
    }
}

Object* findObject(ObjectNode* objList, char* name) {
    while (objList != NULL) {
        if (strcmp(objList->object->name, name) == 0)
            return objList->object;
        objList = objList->next;
    }
    return NULL;
}


/******************* Symbol Table Operations ******************************/
void initSymTab(void) {
    symbolTable = allocateMemory(sizeof(SymTab));
    symbolTable->program = NULL;
    symbolTable->currentScope = NULL;
    symbolTable->globalObjectList = NULL;
    
    globalIntType = makeIntType();
    globalCharType = makeCharType();
    
    Object* obj;
    
    // Built-in function READC
    obj = createFunctionObject("READC");
    obj->funcAttrs->returnType = makeCharType();
    addObject(&(symbolTable->globalObjectList), obj);
    
    // Built-in function READI
    obj = createFunctionObject("READI");
    obj->funcAttrs->returnType = makeIntType();
    addObject(&(symbolTable->globalObjectList), obj);
    
    // Built-in procedure WRITEI
    obj = createProcedureObject("WRITEI");
    Object* param = createParameterObject("i", PARAM_VALUE, obj);
    param->paramAttrs->type = makeIntType();
    addObject(&(obj->procAttrs->paramList), param);
    addObject(&(symbolTable->globalObjectList), obj);
    
    // Built-in procedure WRITEC
    obj = createProcedureObject("WRITEC");
    param = createParameterObject("ch", PARAM_VALUE, obj);
    param->paramAttrs->type = makeCharType();
    addObject(&(obj->procAttrs->paramList), param);
    addObject(&(symbolTable->globalObjectList), obj);
    
    // Built-in procedure WRITELN
    obj = createProcedureObject("WRITELN");
    addObject(&(symbolTable->globalObjectList), obj);
}

void declareObject(Object* obj) {
    if (!obj || !symbolTable->currentScope) return;
    
    if (obj->kind == OBJ_PARAMETER) {
        Object* owner = symbolTable->currentScope->owner;
        switch (owner->kind) {
            case OBJ_FUNCTION:
                addObject(&(owner->funcAttrs->paramList), obj);
                break;
            case OBJ_PROCEDURE:
                addObject(&(owner->procAttrs->paramList), obj);
                break;
            default:
                break;
        }
    } else {
        addObject(&(symbolTable->currentScope->objList), obj);
    }
}

/******************* Memory Cleanup ******************************/
void cleanSymTab(void) {
    if (!symbolTable) return;
    
    freeObject(symbolTable->program);
    freeObjectList(symbolTable->globalObjectList);
    freeType(globalIntType);
    freeType(globalCharType);
    free(symbolTable);
    symbolTable = NULL;
}

void freeConstantValue(ConstantValue* value) {
    if (value == NULL) return;
    free(value);
}

void freeType(Type* type) {
    if (type == NULL) return;
    if (type->typeClass == TP_ARRAY)
        freeType(type->elementType);
    free(type);
}

void freeScope(Scope* scope) {
    if (scope == NULL) return;
    freeObjectList(scope->objList);
    free(scope);
}

void freeObjectList(ObjectNode* objList) {
    if (objList == NULL) return;
    freeObjectList(objList->next);
    freeObject(objList->object);
    free(objList);
}

void freeObject(Object* obj) {
    if (obj == NULL) return;
    switch (obj->kind) {
        case OBJ_CONSTANT:
            freeConstantValue(obj->constAttrs->value);
            free(obj->constAttrs);
            break;
        case OBJ_TYPE:
            freeType(obj->typeAttrs->actualType);
            free(obj->typeAttrs);
            break;
        case OBJ_VARIABLE:
            freeType(obj->varAttrs->type);
            free(obj->varAttrs);
            break;
        case OBJ_FUNCTION:
            freeType(obj->funcAttrs->returnType);
            freeObjectList(obj->funcAttrs->paramList);
            freeScope(obj->funcAttrs->scope);
            free(obj->funcAttrs);
            break;
        case OBJ_PROCEDURE:
            freeObjectList(obj->procAttrs->paramList);
            freeScope(obj->procAttrs->scope);
            free(obj->procAttrs);
            break;
        case OBJ_PROGRAM:
            freeScope(obj->progAttrs->scope);
            free(obj->progAttrs);
            break;
        case OBJ_PARAMETER:
            freeType(obj->paramAttrs->type);
            free(obj->paramAttrs);
            break;
    }
    free(obj);
}