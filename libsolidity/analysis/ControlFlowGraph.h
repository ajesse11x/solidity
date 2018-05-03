/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <libsolidity/ast/AST.h>
#include <libsolidity/ast/ASTVisitor.h>
#include <libsolidity/interface/ErrorReporter.h>

#include <map>
#include <memory>
#include <stack>
#include <vector>

namespace dev
{
namespace solidity
{

struct CFGNode
{
	std::vector<CFGNode*> entries;
	std::vector<CFGNode*> exits;
	std::vector<ASTNode const*> astNodes;
};

struct FunctionFlow
{
	CFGNode* entry;
	CFGNode* exit;
	CFGNode* exception;
};

struct ModifierFlow
{
	CFGNode* entry;
	CFGNode* exit;
	CFGNode* exception;
	std::vector<std::pair<CFGNode*, CFGNode*>> placeholders;
};

class CFG: private ASTConstVisitor
{
public:
	explicit CFG(ErrorReporter& _errorReporter): m_errorReporter(_errorReporter) {}

	bool checkFlow(ASTNode const& _astRoot);

	virtual bool visit(ModifierDefinition const& _modifier) override;
	virtual void endVisit(ModifierDefinition const&) override;
	virtual bool visit(FunctionDefinition const& _function) override;
	virtual void endVisit(FunctionDefinition const& _function) override;
	virtual bool visit(IfStatement const& _ifStatement) override;
	virtual bool visit(ForStatement const& _forStatement) override;
	virtual bool visit(WhileStatement const& _whileStatement) override;
	virtual bool visit(Break const&) override;
	virtual bool visit(Continue const&) override;
	virtual bool visit(Throw const&) override;
	virtual bool visit(Block const&) override;
	virtual void endVisit(Block const&) override;
	virtual bool visit(Return const& _return) override;
	virtual bool visit(PlaceholderStatement const&) override;

protected:
	virtual bool visitNode(ASTNode const& node) override;


private:
	CFGNode* newNode();
	static void addEdge(CFGNode* _from, CFGNode* _to);
	void checkUnassignedStorageReturnValues(
		FunctionDefinition const& _function,
		CFGNode const* _functionEntry,
		CFGNode const* _functionExit
	) const;

	ErrorReporter& m_errorReporter;

	CFGNode* m_returnJump = nullptr;
	CFGNode* m_exceptionJump = nullptr;
	std::stack<CFGNode*> m_breakJumps;
	std::stack<CFGNode*> m_continueJumps;

	std::map<FunctionDefinition const*, std::shared_ptr<FunctionFlow>> m_functionControlFlow;
	std::shared_ptr<FunctionFlow> m_currentFunctionFlow;

	std::map<ModifierDefinition const*, std::shared_ptr<ModifierFlow>> m_modifierControlFlow;
	std::shared_ptr<ModifierFlow> m_currentModifierFlow;

	CFGNode* m_currentNode = nullptr;
	std::vector<std::unique_ptr<CFGNode>> m_nodes;
};

}
}
