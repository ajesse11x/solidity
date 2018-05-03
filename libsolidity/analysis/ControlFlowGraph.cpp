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

#include <libsolidity/analysis/ControlFlowGraph.h>

#include <algorithm>

using namespace std;
using namespace dev::solidity;

bool CFG::checkFlow(ASTNode const& _astRoot)
{
	_astRoot.accept(*this);
	return Error::containsOnlyWarnings(m_errorReporter.errors());
}

bool CFG::visit(ModifierDefinition const& _modifier)
{
	solAssert(!m_currentNode, "");
	solAssert(!m_currentFunctionFlow, "");
	m_currentModifierFlow = shared_ptr<ModifierFlow>(new ModifierFlow { newNode(), newNode(), newNode(), {} });
	m_modifierControlFlow[&_modifier] = m_currentModifierFlow;
	m_currentNode = m_currentModifierFlow->entry;
	m_returnJump = m_currentModifierFlow->exit;
	m_exceptionJump = m_currentModifierFlow->exception;
	return true;
}

void CFG::endVisit(ModifierDefinition const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_currentModifierFlow, "");
	solAssert(m_currentModifierFlow->exit, "");
	addEdge(m_currentNode, m_currentModifierFlow->exit);
	m_currentNode = nullptr;
	m_returnJump = nullptr;
	m_exceptionJump = nullptr;
	m_currentModifierFlow.reset();
}

bool CFG::visit(FunctionDefinition const& _function)
{
	// TODO: insert modifiers into function control flow

	solAssert(!m_currentNode, "");
	solAssert(!m_currentFunctionFlow, "");

	m_currentFunctionFlow = shared_ptr<FunctionFlow>(new FunctionFlow { newNode(), newNode(), newNode() });
	m_functionControlFlow[&_function] = m_currentFunctionFlow;

	m_currentNode = m_currentFunctionFlow->entry;
	m_returnJump = m_currentFunctionFlow->exit;
	m_exceptionJump = m_currentFunctionFlow->exception;
	return true;
}

void CFG::endVisit(FunctionDefinition const& _function)
{
	solAssert(!!m_currentNode, "");
	solAssert(!!m_currentFunctionFlow, "");
	solAssert(m_currentFunctionFlow->entry, "");
	solAssert(m_currentFunctionFlow->exit, "");
	addEdge(m_currentNode, m_currentFunctionFlow->exit);

	checkUnassignedStorageReturnValues(_function, m_currentFunctionFlow->entry, m_currentFunctionFlow->exit);

	m_currentNode = nullptr;
	m_returnJump = nullptr;
	m_exceptionJump = nullptr;
	m_currentFunctionFlow.reset();
}

bool CFG::visit(IfStatement const& _ifStatement)
{
	solAssert(!!m_currentNode, "");

	_ifStatement.condition().accept(*this);

	auto beforeBranch = m_currentNode;
	auto afterBranch = newNode();

	m_currentNode = newNode();
	addEdge(beforeBranch, m_currentNode);
	_ifStatement.trueStatement().accept(*this);
	addEdge(m_currentNode, afterBranch);

	if (_ifStatement.falseStatement())
	{
		m_currentNode = newNode();
		addEdge(beforeBranch, m_currentNode);
		_ifStatement.falseStatement()->accept(*this);
		addEdge(m_currentNode, afterBranch);
	}
	else
		addEdge(beforeBranch, afterBranch);


	m_currentNode = afterBranch;

	return false;
}

bool CFG::visit(ForStatement const& _forStatement)
{
	solAssert(!!m_currentNode, "");

	if (auto initializationExpression = _forStatement.initializationExpression())
		initializationExpression->accept(*this);

	auto condition = newNode();
	auto body = newNode();
	auto loopExpression = newNode();
	auto afterFor = newNode();

	addEdge(m_currentNode, condition);
	m_currentNode = condition;

	if (auto conditionExpression = _forStatement.condition())
		conditionExpression->accept(*this);

	addEdge(m_currentNode, body);
	addEdge(m_currentNode, afterFor);

	m_currentNode = body;
	m_breakJumps.push(afterFor);
	m_continueJumps.push(loopExpression);
	_forStatement.body().accept(*this);
	m_breakJumps.pop();
	m_continueJumps.pop();

	addEdge(m_currentNode, loopExpression);
	m_currentNode = loopExpression;

	if (auto expression = _forStatement.loopExpression())
		expression->accept(*this);

	addEdge(m_currentNode, condition);

	m_currentNode = afterFor;

	return false;
}

bool CFG::visit(WhileStatement const& _whileStatement)
{
	solAssert(!!m_currentNode, "");

	auto afterWhile = newNode();
	if (_whileStatement.isDoWhile())
	{
		auto whileBody = newNode();

		addEdge(m_currentNode, whileBody);
		m_currentNode = whileBody;

		m_continueJumps.push(whileBody);
		m_breakJumps.push(afterWhile);
		_whileStatement.body().accept(*this);
		m_breakJumps.pop();
		m_continueJumps.pop();
		_whileStatement.condition().accept(*this);

		addEdge(m_currentNode, afterWhile);
		addEdge(m_currentNode, whileBody);
	}
	else
	{
		auto whileCondition = newNode();
		addEdge(m_currentNode, whileCondition);
		_whileStatement.condition().accept(*this);
		auto whileBody = newNode();
		addEdge(m_currentNode, whileBody);
		addEdge(m_currentNode, afterWhile);

		m_currentNode = whileBody;
		m_breakJumps.push(afterWhile);
		m_continueJumps.push(whileCondition);
		_whileStatement.body().accept(*this);
		m_breakJumps.pop();
		m_continueJumps.pop();

		addEdge(m_currentNode, whileCondition);

	}

	m_currentNode = afterWhile;

	return false;
}

bool CFG::visit(Break const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!m_breakJumps.empty(), "");
	addEdge(m_currentNode, m_breakJumps.top());
	m_currentNode = newNode();
	return false;
}

bool CFG::visit(Continue const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(!m_continueJumps.empty(), "");
	addEdge(m_currentNode, m_continueJumps.top());
	m_currentNode = newNode();
	return false;
}

bool CFG::visit(Throw const&)
{
	solAssert(!!m_currentNode, "");
	solAssert(m_exceptionJump, "");
	addEdge(m_currentNode, m_exceptionJump);
	m_currentNode = newNode();
	return false;
}

bool CFG::visit(Block const&)
{
	solAssert(!!m_currentNode, "");
	auto beforeBlock = m_currentNode;
	m_currentNode = newNode();
	addEdge(beforeBlock, m_currentNode);
	return true;
}

void CFG::endVisit(Block const&)
{
	solAssert(!!m_currentNode, "");
	auto blockEnd = m_currentNode;
	m_currentNode = newNode();
	addEdge(blockEnd, m_currentNode);
}

bool CFG::visit(Return const& _return)
{
	ASTConstVisitor::visit(_return);
	solAssert(m_returnJump, "");
	addEdge(m_currentNode, m_returnJump);
	m_currentNode = newNode();
	return true;
}


bool CFG::visit(PlaceholderStatement const&)
{
	solAssert(m_currentModifierFlow, "");
	auto placeholderEntry = newNode();
	auto placeholderExit = newNode();

	addEdge(m_currentNode, placeholderEntry);

	m_currentModifierFlow->placeholders.emplace_back(placeholderEntry, placeholderExit);

	m_currentNode = placeholderExit;
	return false;
}

bool CFG::visitNode(ASTNode const& node)
{
	if (m_currentNode)
	{
		if (auto const* statement = dynamic_cast<Statement const*>(&node))
			m_currentNode->astNodes.emplace_back(statement);
		if (auto const* expression = dynamic_cast<Expression const*>(&node))
			m_currentNode->astNodes.emplace_back(expression);
	}
	return true;
}

CFGNode* CFG::newNode()
{
	m_nodes.emplace_back(new CFGNode());
	return m_nodes.back().get();
}

void CFG::addEdge(CFGNode* _from, CFGNode* _to)
{
	solAssert(_from, "");
	solAssert(_to, "");
	_from->exits.push_back(_to);
	_to->entries.push_back(_from);
}

void CFG::checkUnassignedStorageReturnValues(
	FunctionDefinition const& _function,
	CFGNode const* _functionEntry,
	CFGNode const* _functionExit
) const
{
	map<CFGNode const*, set<VariableDeclaration const*>> unassigned;

	{
		auto& unassignedAtFunctionEntry = unassigned[_functionEntry];
		for (auto const& returnParameter: _function.returnParameterList()->parameters())
			if (returnParameter->type()->dataStoredIn(DataLocation::Storage))
				unassignedAtFunctionEntry.insert(returnParameter.get());
	}

	stack<CFGNode const*> nodesToTraverse;
	nodesToTraverse.push(_functionEntry);

	while (!nodesToTraverse.empty())
	{
		auto node = nodesToTraverse.top();
		nodesToTraverse.pop();

		auto& unassignedAtNode = unassigned[node];

		for (auto astNode: node->astNodes)
		{
			if (dynamic_cast<Return const*>(astNode))
			{
				unassignedAtNode.clear();
				break;
			}
			else if (auto const* assignment = dynamic_cast<Assignment const*>(astNode))
			{
				stack<Expression const*> expressions;
				expressions.push(&assignment->leftHandSide());
				while (!expressions.empty())
				{
					Expression const* expression = expressions.top();
					expressions.pop();

					if (auto const *tuple = dynamic_cast<TupleExpression const*>(expression))
						for (auto const& component: tuple->components())
							expressions.push(component.get());
					else if (auto const* identifier = dynamic_cast<Identifier const*>(expression))
						if (auto const* variableDeclaration = dynamic_cast<VariableDeclaration const*>(
							identifier->annotation().referencedDeclaration
						))
						{
							unassignedAtNode.erase(variableDeclaration);
							if (unassignedAtNode.empty())
								break;
						}
				}
			}
		}

		for (auto const& exit: node->exits)
		{
			auto& unassignedAtExit = unassigned[exit];
			if (unassignedAtNode.size() > unassignedAtExit.size())
			{
				unassignedAtExit.insert(unassignedAtNode.begin(), unassignedAtNode.end());
				nodesToTraverse.push(exit);
			}
		}
	}

	if (unassigned[_functionExit].size() > 0)
	{
		vector<VariableDeclaration const*> unassignedOrdered(
			unassigned[_functionExit].begin(),
			unassigned[_functionExit].end()
		);
		sort(
			unassignedOrdered.begin(),
			unassignedOrdered.end(),
			[](VariableDeclaration const* lhs, VariableDeclaration const* rhs) -> bool {
				return lhs->id() < rhs->id();
			}
		);
		for (auto returnVal: unassignedOrdered)
			m_errorReporter.warning(returnVal->location(), "uninitialized storage pointer may be returned");
	}
}
