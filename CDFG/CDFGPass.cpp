#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <llvm/Analysis/CFG.h>
#include <list>
#include <map>

using namespace llvm;

namespace {
struct CDFGPass : public ModulePass {
public:
    typedef std::pair<Value*, StringRef> node;
    typedef std::pair<node, node> edge;
    typedef std::list<node> node_list;
    typedef std::list<edge> edge_list;
    static char ID;
    
    edge_list dataFlowEdges;  
    edge_list controlFlowEdges; 
    std::map<Function*, std::map<BasicBlock*, node_list>> functionNodes;
    
    int num;

    CDFGPass() : ModulePass(ID) { num = 0; }

    std::string changeIns2Str(Instruction* ins) {
        std::string temp_str;
        raw_string_ostream os(temp_str);
        ins->print(os);
        return os.str();
    }

    StringRef getValueName(Value* v) {
        std::string temp_result = "val";
        if (!v) {
            return "undefined";
        }
        if (v->getName().empty()) {
            temp_result += std::to_string(num);
            num++;
        } else {
            temp_result = v->getName().str();
        }
        StringRef result(temp_result);
        return result;
    }

    bool runOnModule(Module &M) override {
        std::error_code error;
        enum sys::fs::OpenFlags F_None;
        StringRef fileName("CDFG3.dot");
        raw_fd_ostream file(fileName, error, F_None);

        errs() << "Hello\n";
        dataFlowEdges.clear();
        controlFlowEdges.clear();
        functionNodes.clear();

        for (Module::iterator FI = M.begin(), FE = M.end(); FI != FE; ++FI) {
            Function &F = *FI;
            if (F.isDeclaration()) continue;

            std::map<BasicBlock*, node_list> &basicBlockNodes = functionNodes[&F];

            for (Function::iterator BBI = F.begin(), BBE = F.end(); BBI != BBE; ++BBI) {
                BasicBlock *curBB = &*BBI;
                node_list &nodes = basicBlockNodes[curBB];

                for (BasicBlock::iterator II = curBB->begin(), IE = curBB->end(); II != IE; ++II) {
                    Instruction* curII = &*II;
                    nodes.push_back(node(curII, getValueName(curII)));

                    if (CallBase* callInst = dyn_cast<CallBase>(curII)) {
                        Function* calledFunc = callInst->getCalledFunction();
                        if (calledFunc && !calledFunc->isDeclaration()) {
                            controlFlowEdges.push_back(edge(node(curII, getValueName(curII)), node(&*(calledFunc->begin()->begin()), getValueName(&*(calledFunc->begin()->begin())))));
                        }
                    }

                    switch (curII->getOpcode()) {
                        case llvm::Instruction::Load: {
                            LoadInst* linst = dyn_cast<LoadInst>(curII);
                            Value* loadValPtr = linst->getPointerOperand();
                            dataFlowEdges.push_back(edge(node(loadValPtr, getValueName(loadValPtr)), node(curII, getValueName(curII))));
                            break;
                        }
                        case llvm::Instruction::Store: {
                            StoreInst* sinst = dyn_cast<StoreInst>(curII);
                            Value* storeValPtr = sinst->getPointerOperand();
                            Value* storeVal = sinst->getValueOperand();
                            dataFlowEdges.push_back(edge(node(storeVal, getValueName(storeVal)), node(curII, getValueName(curII))));
                            dataFlowEdges.push_back(edge(node(curII, getValueName(curII)), node(storeValPtr, getValueName(storeValPtr))));
                            break;
                        }
                        default: {
                            for (Instruction::op_iterator op = curII->op_begin(), opEnd = curII->op_end(); op != opEnd; ++op) {
                                if (dyn_cast<Instruction>(*op)) {
                                    dataFlowEdges.push_back(edge(node(op->get(), getValueName(op->get())), node(curII, getValueName(curII))));
                                }
                            }
                            break;
                        }
                    }
                }
            }

            for (Function::iterator BBI = F.begin(), BBE = F.end(); BBI != BBE; ++BBI) {
                BasicBlock *curBB = &*BBI;
                Instruction* terminator = curBB->getTerminator();
                for (BasicBlock* sucBB : successors(curBB)) {
                    Instruction* first = &*(sucBB->begin());
                    controlFlowEdges.push_back(edge(node(terminator, getValueName(terminator)), node(first, getValueName(first))));
                }
            }
        }

        file << "digraph \"CDFG for Module\" {\n";
        for (auto &FN : functionNodes) {
            Function *F = FN.first;
            std::map<BasicBlock*, node_list> &basicBlockNodes = FN.second;
            file << "subgraph cluster_" << F->getName().str() << " {\n";
            file << "label = \"" << F->getName().str() << "\";\n";
            for (auto &BBN : basicBlockNodes) {
                BasicBlock *BB = BBN.first;
                node_list &nodes = BBN.second;
                file << "subgraph cluster_" << BB << " {\n";
                file << "label = \"BB_" << BB->getName().str() << "\";\n";
                for (node_list::iterator NI = nodes.begin(), NE = nodes.end(); NI != NE; ++NI) {
                    if (dyn_cast<Instruction>(NI->first))
                        file << "\tNode" << NI->first << "[shape=record, label=\"" << *(NI->first) << "\"];\n";
                    else
                        file << "\tNode" << NI->first << "[shape=record, label=\"" << NI->second << "\"];\n";
                }
                file << "}\n";
            }
            file << "}\n";
        }

        //control flow edges
        file << "edge [color=black]" << "\n";
        for (edge_list::iterator EI = controlFlowEdges.begin(), EE = controlFlowEdges.end(); EI != EE; ++EI) {
            file << "\tNode" << EI->first.first << " -> Node" << EI->second.first << "\n";
        }

        //data flow edges
        file << "edge [color=red]" << "\n";
        for (edge_list::iterator EI = dataFlowEdges.begin(), EE = dataFlowEdges.end(); EI != EE; ++EI) {
            file << "\tNode" << EI->first.first << " -> Node" << EI->second.first << "\n";
        }

        errs() << "Write Done\n";
        file << "}\n";
        file.close();
        return false;
    }
};
}

char CDFGPass::ID = 0;
static RegisterPass<CDFGPass> X("CDFGPass", "CDFG Pass Analyse",
    false, false
);

