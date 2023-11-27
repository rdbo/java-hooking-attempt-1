#include "jni.h"
#include <iostream>
#include <stdint.h>
#include <libmem/libmem.hpp>

#define JNI_VERSION JNI_VERSION_1_8

struct BasicHashtableEntry {
	unsigned int _hash;
	void *_next;
};

typedef void *address;

struct AdapterHandlerEntry : BasicHashtableEntry {
	void *_fingerprint;
	address _i2c_entry;
	address _c2i_entry;
};

struct Method {
	void **Metadata_vtable;
	void *_constMethod;
	void *_method_data;
	void *_method_counters;
	AdapterHandlerEntry *_adapter;
	int   _access_flags;
	int   _vtable_index;
	uint16_t  _intrinsic_id;
	uint16_t  _flags;

	void *_i2i_entry;
	void *_from_compiled_entry;
	void *_code;
	void *_from_interpreted_entry;
};

#define THREAD __the_thread__
#define TRAPS void *THREAD

typedef int BasicType;

/*
typedef void (*CallStub)(
    void     *link,
    intptr_t *result,
    BasicType result_type,
    Method   *method,
    void     *entry_point,
    intptr_t *parameters,
    int       size_of_parameters,
    TRAPS
  );
*/

typedef void (*CallStub)(void *, void *, void *, void *, void *, void *, void *, void *);
static CallStub orig_stub;

//void hookStub(void *link, intptr_t *result, BasicType result_type, Method *method, void *entry_point, intptr_t *parameters, int size_of_parameters, TRAPS)
//void hookStub()
//void hookStub(void *link, intptr_t *result, void *result_type, Method *method, void *entry_point, intptr_t *parameters, void *size_of_parameters, void *__the_thread__)
void hookStub(Method *method, intptr_t *parameters)
{
	std::cout << "hookStub called!" << std::endl;
	std::cout << "Stub info: " << std::endl;
	// std::cout << "  link: " << link << std::endl;
	// std::cout << "  result_type: " << result_type << std::endl;
	std::cout << "  method: " << method << std::endl;
	// std::cout << "  entry_point: " << entry_point << std::endl;
	std::cout << "  parameters: " << parameters << std::endl;
	// std::cout << "  size_of_parameters: " << size_of_parameters << std::endl;
	// std::cout << "  _THREAD: " << THREAD << std::endl;

	// orig_stub(link, result, result_type, method, entry_point, parameters, size_of_parameters, THREAD);
}

const char *GATEWAY_MSG = "STUB GATEWAY CALLED\n";

/*
__attribute__((naked))
void hookStubGateway()
{
	asm volatile(
		"push %%rax\n"
		"push %%rbx\n"
		"push %%rcx\n"
		"push %%rdx\n"
		"push %%rsi\n"
		"push %%rdi\n"
		"push %%r8\n"
		"push %%r9\n"
		"push %%r10\n"
		"push %%r11\n"
		"push %%rbp\n"
		"push %%rsp\n"

		"mov %%rdi, %%r11\n"
		"mov %2, %%rdi\n"
		"call *%3\n"

		"mov %%r11, %%rdi\n"
		"call *%0\n"
		
		"pop %%rsp\n"
		"pop %%rbp\n"
		"pop %%r11\n"
		"pop %%r10\n"
		"pop %%r9\n"
		"pop %%r8\n"
		"pop %%rdi\n"
		"pop %%rsi\n"
		"pop %%rdx\n"
		"pop %%rcx\n"
		"pop %%rbx\n"
		"pop %%rax\n"

		"jmp *%0\n"
		:
		: "r" (hookStub), "r" (orig_stub), "r" (GATEWAY_MSG), "r" (printf)
	);
}
*/

lm_address_t hookStubGateway;

void dump_code(void *addr, size_t count)
{
	lm_inst_t *insts;
	lm_size_t  inst_count;
	
	inst_count = LM_DisassembleEx((lm_address_t)addr, LM_BITS, 0x1000, count, (lm_address_t)addr, &insts);
	for (size_t i = 0; i < inst_count; ++i) {
		std::cout << "  " << insts[i].mnemonic << " " << insts[i].op_str << std::endl;
	}
	LM_FreeInstructions(insts);
}

void __attribute__((constructor))
dl_entry()
{
	JavaVM *jvm;
	JNIEnv *env;
	JavaVMAttachArgs attach_args;

	std::cout << "Library loaded!" << std::endl;

	std::cout << "JNI_GetCreatedJavaVMs: " << JNI_GetCreatedJavaVMs(&jvm, 1, NULL) << std::endl;
	std::cout << "JavaVM pointer: " << (void *)jvm << std::endl;

	attach_args.version = JNI_VERSION;
	attach_args.name = nullptr;
	attach_args.group = nullptr;

	jvm->AttachCurrentThread((void **)&env, (void *)&attach_args);

	std::cout << "JNIEnv pointer: " << (void *)env << std::endl;

	jclass main = env->FindClass("main/Main");
	printf("Main class pointer: %p\n", (void *)main);

	jmethodID hookMeID = env->GetStaticMethodID(main, "hookMe", "(I)V");
	env->CallStaticVoidMethod(main, hookMeID);

	Method *method = *(Method **)hookMeID;
	std::cout << "Method address: " << (void *)method << std::endl;

	std::cout << "i2i entry of method: " << method->_i2i_entry << std::endl;
	std::cout << "from_interpreted_entry of method: " << method->_from_interpreted_entry << std::endl;
	std::cout << "adapter _i2c_entry: " << method->_adapter->_i2c_entry << std::endl;
	std::cout << "adapter _c2i_entry: " << method->_adapter->_c2i_entry << std::endl;

	//std::cout << "press [ENTER] to continue..." << std::endl;
	//scanf("%*c");

	/*
	std::cout << "Dumping code of i2c_entry stub: " << std::endl;
	std::cout << "--START--" << std::endl;
	dump_code(method->_from_interpreted_entry, 100);
	std::cout << "--END--" << std::endl;
	*/

	orig_stub = (CallStub)method->_from_interpreted_entry;

	char codebuf[4096] = { 0 };
	snprintf(
		codebuf, sizeof(codebuf),
		"push rbx\n"
		"push rcx\n"
		"push rdx\n"
		"push rsi\n"
		"push rdi\n"
		"push r8\n"
		"push r9\n"
		"push r10\n"
		"push r11\n"
		"push rbp\n"
		"push rsp\n"

		"mov rdi, [rbp - 24]\n"
		"mov rsi, [rbp - 8]\n"
		"mov rax, %p\n"
		"call rax\n"

		"pop rsp\n"
		"pop rbp\n"
		"pop r11\n"
		"pop r10\n"
		"pop r9\n"
		"pop r8\n"
		"pop rdi\n"
		"pop rsi\n"
		"pop rdx\n"
		"pop rcx\n"
		"pop rbx\n"

		"mov rax, %p\n"
		"jmp rax",
		(void *)hookStub,
		(void *)orig_stub
	);

	hookStubGateway = LM_AllocMemory(0x1000, LM_PROT_XRW);
	lm_bytearr_t code;
	lm_size_t codesize = LM_AssembleEx(codebuf, LM_BITS, hookStubGateway, &code);
	LM_WriteMemory(hookStubGateway, code, codesize);

	std::cout << "gateway code: " << std::endl;
	dump_code((void *)hookStubGateway, 50);

	method->_from_interpreted_entry = (void *)hookStubGateway;
	// method->_adapter->_i2c_entry = (void *)hookStub;
	//LM_HookCode((lm_address_t)method->_adapter->_i2c_entry, (lm_address_t)hookStub, (lm_address_t *)&orig_stub);

	// dump_code((void *)hookStubGateway, 5);

	std::cout << "Calling hookMe to see if it works..." << std::endl;

	env->CallStaticVoidMethod(main, hookMeID, 20);

	std::cout << "Call finished" << std::endl;
}
