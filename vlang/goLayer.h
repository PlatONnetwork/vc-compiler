#pragma once

#include <stdlib.h>

typedef int	int32;
typedef unsigned int uint32;
typedef long long	int64;
typedef unsigned long long	uint64;


typedef struct tagNode {
	int32  type;
	uint64 Input[4];
	uint64 Result;
}SSA_Node;	

typedef enum eType {
	G_ADD  = 0,
	G_SUB,
	G_MUL,
	G_SDIV,
	G_SREM,
	G_UDIV,
	G_UREM,
	G_AND,
	G_OR,
	G_NOT,
	G_SELECT,
	G_BITW_OR,
	G_BITW_XOR,
	G_BITW_AND,
	G_TRUNC,
	G_ZEXT,
	G_SEXT,
	G_EQ,
	G_NEQ,
	G_SGT,
	G_SGE,
	G_UGT,
	G_UGE,
}E_GType;


extern "C" 
{
  extern int64_t RetIndex;

	void gadget_initEnv();
	void gadget_uninitEnv();


	uint64 gadget_createPBVar(int64_t ptr);
	unsigned char gadget_createGadget(int64_t input0, int64_t input1, int64_t input2, int64_t result, int32 Type);
	void gadget_setVar(int64_t ptr, uint64 Val);
	long gadget_getVar(int64_t ptr);


	void gadget_generateConstraints();
	void gadget_generateWitness();
	unsigned char GenerateProof(const char *pPKEY, char *pProof, unsigned prSize);
  unsigned char GenerateResult(int64_t RetIndex, char *pResult, unsigned resSize);

  unsigned char GenerateProofAndResult(const char *pPKEY, char *pProof, unsigned prSize, 
                          char *pResult, unsigned resSize);

	unsigned char Verify(const char *pVKEY, const char *pPoorf, 
						const char *pInput, const char *pOutput);
}



