// Code generated from FhirPath.g4 by ANTLR 4.13.1. DO NOT EDIT.

package gen // FhirPath
import "github.com/antlr4-go/antlr/v4"

// BaseFhirPathListener is a complete listener for a parse tree produced by FhirPathParser.
type BaseFhirPathListener struct{}

var _ FhirPathListener = &BaseFhirPathListener{}

// VisitTerminal is called when a terminal node is visited.
func (s *BaseFhirPathListener) VisitTerminal(node antlr.TerminalNode) {}

// VisitErrorNode is called when an error node is visited.
func (s *BaseFhirPathListener) VisitErrorNode(node antlr.ErrorNode) {}

// EnterEveryRule is called when any rule is entered.
func (s *BaseFhirPathListener) EnterEveryRule(ctx antlr.ParserRuleContext) {}

// ExitEveryRule is called when any rule is exited.
func (s *BaseFhirPathListener) ExitEveryRule(ctx antlr.ParserRuleContext) {}

// EnterIndexerExpression is called when production indexerExpression is entered.
func (s *BaseFhirPathListener) EnterIndexerExpression(ctx *IndexerExpressionContext) {}

// ExitIndexerExpression is called when production indexerExpression is exited.
func (s *BaseFhirPathListener) ExitIndexerExpression(ctx *IndexerExpressionContext) {}

// EnterPolarityExpression is called when production polarityExpression is entered.
func (s *BaseFhirPathListener) EnterPolarityExpression(ctx *PolarityExpressionContext) {}

// ExitPolarityExpression is called when production polarityExpression is exited.
func (s *BaseFhirPathListener) ExitPolarityExpression(ctx *PolarityExpressionContext) {}

// EnterAdditiveExpression is called when production additiveExpression is entered.
func (s *BaseFhirPathListener) EnterAdditiveExpression(ctx *AdditiveExpressionContext) {}

// ExitAdditiveExpression is called when production additiveExpression is exited.
func (s *BaseFhirPathListener) ExitAdditiveExpression(ctx *AdditiveExpressionContext) {}

// EnterMultiplicativeExpression is called when production multiplicativeExpression is entered.
func (s *BaseFhirPathListener) EnterMultiplicativeExpression(ctx *MultiplicativeExpressionContext) {}

// ExitMultiplicativeExpression is called when production multiplicativeExpression is exited.
func (s *BaseFhirPathListener) ExitMultiplicativeExpression(ctx *MultiplicativeExpressionContext) {}

// EnterUnionExpression is called when production unionExpression is entered.
func (s *BaseFhirPathListener) EnterUnionExpression(ctx *UnionExpressionContext) {}

// ExitUnionExpression is called when production unionExpression is exited.
func (s *BaseFhirPathListener) ExitUnionExpression(ctx *UnionExpressionContext) {}

// EnterOrExpression is called when production orExpression is entered.
func (s *BaseFhirPathListener) EnterOrExpression(ctx *OrExpressionContext) {}

// ExitOrExpression is called when production orExpression is exited.
func (s *BaseFhirPathListener) ExitOrExpression(ctx *OrExpressionContext) {}

// EnterAndExpression is called when production andExpression is entered.
func (s *BaseFhirPathListener) EnterAndExpression(ctx *AndExpressionContext) {}

// ExitAndExpression is called when production andExpression is exited.
func (s *BaseFhirPathListener) ExitAndExpression(ctx *AndExpressionContext) {}

// EnterMembershipExpression is called when production membershipExpression is entered.
func (s *BaseFhirPathListener) EnterMembershipExpression(ctx *MembershipExpressionContext) {}

// ExitMembershipExpression is called when production membershipExpression is exited.
func (s *BaseFhirPathListener) ExitMembershipExpression(ctx *MembershipExpressionContext) {}

// EnterInequalityExpression is called when production inequalityExpression is entered.
func (s *BaseFhirPathListener) EnterInequalityExpression(ctx *InequalityExpressionContext) {}

// ExitInequalityExpression is called when production inequalityExpression is exited.
func (s *BaseFhirPathListener) ExitInequalityExpression(ctx *InequalityExpressionContext) {}

// EnterInvocationExpression is called when production invocationExpression is entered.
func (s *BaseFhirPathListener) EnterInvocationExpression(ctx *InvocationExpressionContext) {}

// ExitInvocationExpression is called when production invocationExpression is exited.
func (s *BaseFhirPathListener) ExitInvocationExpression(ctx *InvocationExpressionContext) {}

// EnterEqualityExpression is called when production equalityExpression is entered.
func (s *BaseFhirPathListener) EnterEqualityExpression(ctx *EqualityExpressionContext) {}

// ExitEqualityExpression is called when production equalityExpression is exited.
func (s *BaseFhirPathListener) ExitEqualityExpression(ctx *EqualityExpressionContext) {}

// EnterImpliesExpression is called when production impliesExpression is entered.
func (s *BaseFhirPathListener) EnterImpliesExpression(ctx *ImpliesExpressionContext) {}

// ExitImpliesExpression is called when production impliesExpression is exited.
func (s *BaseFhirPathListener) ExitImpliesExpression(ctx *ImpliesExpressionContext) {}

// EnterTermExpression is called when production termExpression is entered.
func (s *BaseFhirPathListener) EnterTermExpression(ctx *TermExpressionContext) {}

// ExitTermExpression is called when production termExpression is exited.
func (s *BaseFhirPathListener) ExitTermExpression(ctx *TermExpressionContext) {}

// EnterTypeExpression is called when production typeExpression is entered.
func (s *BaseFhirPathListener) EnterTypeExpression(ctx *TypeExpressionContext) {}

// ExitTypeExpression is called when production typeExpression is exited.
func (s *BaseFhirPathListener) ExitTypeExpression(ctx *TypeExpressionContext) {}

// EnterInvocationTerm is called when production invocationTerm is entered.
func (s *BaseFhirPathListener) EnterInvocationTerm(ctx *InvocationTermContext) {}

// ExitInvocationTerm is called when production invocationTerm is exited.
func (s *BaseFhirPathListener) ExitInvocationTerm(ctx *InvocationTermContext) {}

// EnterLiteralTerm is called when production literalTerm is entered.
func (s *BaseFhirPathListener) EnterLiteralTerm(ctx *LiteralTermContext) {}

// ExitLiteralTerm is called when production literalTerm is exited.
func (s *BaseFhirPathListener) ExitLiteralTerm(ctx *LiteralTermContext) {}

// EnterExternalConstantTerm is called when production externalConstantTerm is entered.
func (s *BaseFhirPathListener) EnterExternalConstantTerm(ctx *ExternalConstantTermContext) {}

// ExitExternalConstantTerm is called when production externalConstantTerm is exited.
func (s *BaseFhirPathListener) ExitExternalConstantTerm(ctx *ExternalConstantTermContext) {}

// EnterParenthesizedTerm is called when production parenthesizedTerm is entered.
func (s *BaseFhirPathListener) EnterParenthesizedTerm(ctx *ParenthesizedTermContext) {}

// ExitParenthesizedTerm is called when production parenthesizedTerm is exited.
func (s *BaseFhirPathListener) ExitParenthesizedTerm(ctx *ParenthesizedTermContext) {}

// EnterNullLiteral is called when production nullLiteral is entered.
func (s *BaseFhirPathListener) EnterNullLiteral(ctx *NullLiteralContext) {}

// ExitNullLiteral is called when production nullLiteral is exited.
func (s *BaseFhirPathListener) ExitNullLiteral(ctx *NullLiteralContext) {}

// EnterBooleanLiteral is called when production booleanLiteral is entered.
func (s *BaseFhirPathListener) EnterBooleanLiteral(ctx *BooleanLiteralContext) {}

// ExitBooleanLiteral is called when production booleanLiteral is exited.
func (s *BaseFhirPathListener) ExitBooleanLiteral(ctx *BooleanLiteralContext) {}

// EnterStringLiteral is called when production stringLiteral is entered.
func (s *BaseFhirPathListener) EnterStringLiteral(ctx *StringLiteralContext) {}

// ExitStringLiteral is called when production stringLiteral is exited.
func (s *BaseFhirPathListener) ExitStringLiteral(ctx *StringLiteralContext) {}

// EnterNumberLiteral is called when production numberLiteral is entered.
func (s *BaseFhirPathListener) EnterNumberLiteral(ctx *NumberLiteralContext) {}

// ExitNumberLiteral is called when production numberLiteral is exited.
func (s *BaseFhirPathListener) ExitNumberLiteral(ctx *NumberLiteralContext) {}

// EnterDateLiteral is called when production dateLiteral is entered.
func (s *BaseFhirPathListener) EnterDateLiteral(ctx *DateLiteralContext) {}

// ExitDateLiteral is called when production dateLiteral is exited.
func (s *BaseFhirPathListener) ExitDateLiteral(ctx *DateLiteralContext) {}

// EnterDateTimeLiteral is called when production dateTimeLiteral is entered.
func (s *BaseFhirPathListener) EnterDateTimeLiteral(ctx *DateTimeLiteralContext) {}

// ExitDateTimeLiteral is called when production dateTimeLiteral is exited.
func (s *BaseFhirPathListener) ExitDateTimeLiteral(ctx *DateTimeLiteralContext) {}

// EnterTimeLiteral is called when production timeLiteral is entered.
func (s *BaseFhirPathListener) EnterTimeLiteral(ctx *TimeLiteralContext) {}

// ExitTimeLiteral is called when production timeLiteral is exited.
func (s *BaseFhirPathListener) ExitTimeLiteral(ctx *TimeLiteralContext) {}

// EnterQuantityLiteral is called when production quantityLiteral is entered.
func (s *BaseFhirPathListener) EnterQuantityLiteral(ctx *QuantityLiteralContext) {}

// ExitQuantityLiteral is called when production quantityLiteral is exited.
func (s *BaseFhirPathListener) ExitQuantityLiteral(ctx *QuantityLiteralContext) {}

// EnterExternalConstant is called when production externalConstant is entered.
func (s *BaseFhirPathListener) EnterExternalConstant(ctx *ExternalConstantContext) {}

// ExitExternalConstant is called when production externalConstant is exited.
func (s *BaseFhirPathListener) ExitExternalConstant(ctx *ExternalConstantContext) {}

// EnterMemberInvocation is called when production memberInvocation is entered.
func (s *BaseFhirPathListener) EnterMemberInvocation(ctx *MemberInvocationContext) {}

// ExitMemberInvocation is called when production memberInvocation is exited.
func (s *BaseFhirPathListener) ExitMemberInvocation(ctx *MemberInvocationContext) {}

// EnterFunctionInvocation is called when production functionInvocation is entered.
func (s *BaseFhirPathListener) EnterFunctionInvocation(ctx *FunctionInvocationContext) {}

// ExitFunctionInvocation is called when production functionInvocation is exited.
func (s *BaseFhirPathListener) ExitFunctionInvocation(ctx *FunctionInvocationContext) {}

// EnterThisInvocation is called when production thisInvocation is entered.
func (s *BaseFhirPathListener) EnterThisInvocation(ctx *ThisInvocationContext) {}

// ExitThisInvocation is called when production thisInvocation is exited.
func (s *BaseFhirPathListener) ExitThisInvocation(ctx *ThisInvocationContext) {}

// EnterIndexInvocation is called when production indexInvocation is entered.
func (s *BaseFhirPathListener) EnterIndexInvocation(ctx *IndexInvocationContext) {}

// ExitIndexInvocation is called when production indexInvocation is exited.
func (s *BaseFhirPathListener) ExitIndexInvocation(ctx *IndexInvocationContext) {}

// EnterTotalInvocation is called when production totalInvocation is entered.
func (s *BaseFhirPathListener) EnterTotalInvocation(ctx *TotalInvocationContext) {}

// ExitTotalInvocation is called when production totalInvocation is exited.
func (s *BaseFhirPathListener) ExitTotalInvocation(ctx *TotalInvocationContext) {}

// EnterFunction is called when production function is entered.
func (s *BaseFhirPathListener) EnterFunction(ctx *FunctionContext) {}

// ExitFunction is called when production function is exited.
func (s *BaseFhirPathListener) ExitFunction(ctx *FunctionContext) {}

// EnterParamList is called when production paramList is entered.
func (s *BaseFhirPathListener) EnterParamList(ctx *ParamListContext) {}

// ExitParamList is called when production paramList is exited.
func (s *BaseFhirPathListener) ExitParamList(ctx *ParamListContext) {}

// EnterQuantity is called when production quantity is entered.
func (s *BaseFhirPathListener) EnterQuantity(ctx *QuantityContext) {}

// ExitQuantity is called when production quantity is exited.
func (s *BaseFhirPathListener) ExitQuantity(ctx *QuantityContext) {}

// EnterUnit is called when production unit is entered.
func (s *BaseFhirPathListener) EnterUnit(ctx *UnitContext) {}

// ExitUnit is called when production unit is exited.
func (s *BaseFhirPathListener) ExitUnit(ctx *UnitContext) {}

// EnterDateTimePrecision is called when production dateTimePrecision is entered.
func (s *BaseFhirPathListener) EnterDateTimePrecision(ctx *DateTimePrecisionContext) {}

// ExitDateTimePrecision is called when production dateTimePrecision is exited.
func (s *BaseFhirPathListener) ExitDateTimePrecision(ctx *DateTimePrecisionContext) {}

// EnterPluralDateTimePrecision is called when production pluralDateTimePrecision is entered.
func (s *BaseFhirPathListener) EnterPluralDateTimePrecision(ctx *PluralDateTimePrecisionContext) {}

// ExitPluralDateTimePrecision is called when production pluralDateTimePrecision is exited.
func (s *BaseFhirPathListener) ExitPluralDateTimePrecision(ctx *PluralDateTimePrecisionContext) {}

// EnterTypeSpecifier is called when production typeSpecifier is entered.
func (s *BaseFhirPathListener) EnterTypeSpecifier(ctx *TypeSpecifierContext) {}

// ExitTypeSpecifier is called when production typeSpecifier is exited.
func (s *BaseFhirPathListener) ExitTypeSpecifier(ctx *TypeSpecifierContext) {}

// EnterQualifiedIdentifier is called when production qualifiedIdentifier is entered.
func (s *BaseFhirPathListener) EnterQualifiedIdentifier(ctx *QualifiedIdentifierContext) {}

// ExitQualifiedIdentifier is called when production qualifiedIdentifier is exited.
func (s *BaseFhirPathListener) ExitQualifiedIdentifier(ctx *QualifiedIdentifierContext) {}

// EnterIdentifier is called when production identifier is entered.
func (s *BaseFhirPathListener) EnterIdentifier(ctx *IdentifierContext) {}

// ExitIdentifier is called when production identifier is exited.
func (s *BaseFhirPathListener) ExitIdentifier(ctx *IdentifierContext) {}
