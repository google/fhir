// Code generated from FhirPath.g4 by ANTLR 4.13.1. DO NOT EDIT.

package gen // FhirPath
import "github.com/antlr4-go/antlr/v4"

// FhirPathListener is a complete listener for a parse tree produced by FhirPathParser.
type FhirPathListener interface {
	antlr.ParseTreeListener

	// EnterIndexerExpression is called when entering the indexerExpression production.
	EnterIndexerExpression(c *IndexerExpressionContext)

	// EnterPolarityExpression is called when entering the polarityExpression production.
	EnterPolarityExpression(c *PolarityExpressionContext)

	// EnterAdditiveExpression is called when entering the additiveExpression production.
	EnterAdditiveExpression(c *AdditiveExpressionContext)

	// EnterMultiplicativeExpression is called when entering the multiplicativeExpression production.
	EnterMultiplicativeExpression(c *MultiplicativeExpressionContext)

	// EnterUnionExpression is called when entering the unionExpression production.
	EnterUnionExpression(c *UnionExpressionContext)

	// EnterOrExpression is called when entering the orExpression production.
	EnterOrExpression(c *OrExpressionContext)

	// EnterAndExpression is called when entering the andExpression production.
	EnterAndExpression(c *AndExpressionContext)

	// EnterMembershipExpression is called when entering the membershipExpression production.
	EnterMembershipExpression(c *MembershipExpressionContext)

	// EnterInequalityExpression is called when entering the inequalityExpression production.
	EnterInequalityExpression(c *InequalityExpressionContext)

	// EnterInvocationExpression is called when entering the invocationExpression production.
	EnterInvocationExpression(c *InvocationExpressionContext)

	// EnterEqualityExpression is called when entering the equalityExpression production.
	EnterEqualityExpression(c *EqualityExpressionContext)

	// EnterImpliesExpression is called when entering the impliesExpression production.
	EnterImpliesExpression(c *ImpliesExpressionContext)

	// EnterTermExpression is called when entering the termExpression production.
	EnterTermExpression(c *TermExpressionContext)

	// EnterTypeExpression is called when entering the typeExpression production.
	EnterTypeExpression(c *TypeExpressionContext)

	// EnterInvocationTerm is called when entering the invocationTerm production.
	EnterInvocationTerm(c *InvocationTermContext)

	// EnterLiteralTerm is called when entering the literalTerm production.
	EnterLiteralTerm(c *LiteralTermContext)

	// EnterExternalConstantTerm is called when entering the externalConstantTerm production.
	EnterExternalConstantTerm(c *ExternalConstantTermContext)

	// EnterParenthesizedTerm is called when entering the parenthesizedTerm production.
	EnterParenthesizedTerm(c *ParenthesizedTermContext)

	// EnterNullLiteral is called when entering the nullLiteral production.
	EnterNullLiteral(c *NullLiteralContext)

	// EnterBooleanLiteral is called when entering the booleanLiteral production.
	EnterBooleanLiteral(c *BooleanLiteralContext)

	// EnterStringLiteral is called when entering the stringLiteral production.
	EnterStringLiteral(c *StringLiteralContext)

	// EnterNumberLiteral is called when entering the numberLiteral production.
	EnterNumberLiteral(c *NumberLiteralContext)

	// EnterDateLiteral is called when entering the dateLiteral production.
	EnterDateLiteral(c *DateLiteralContext)

	// EnterDateTimeLiteral is called when entering the dateTimeLiteral production.
	EnterDateTimeLiteral(c *DateTimeLiteralContext)

	// EnterTimeLiteral is called when entering the timeLiteral production.
	EnterTimeLiteral(c *TimeLiteralContext)

	// EnterQuantityLiteral is called when entering the quantityLiteral production.
	EnterQuantityLiteral(c *QuantityLiteralContext)

	// EnterExternalConstant is called when entering the externalConstant production.
	EnterExternalConstant(c *ExternalConstantContext)

	// EnterMemberInvocation is called when entering the memberInvocation production.
	EnterMemberInvocation(c *MemberInvocationContext)

	// EnterFunctionInvocation is called when entering the functionInvocation production.
	EnterFunctionInvocation(c *FunctionInvocationContext)

	// EnterThisInvocation is called when entering the thisInvocation production.
	EnterThisInvocation(c *ThisInvocationContext)

	// EnterIndexInvocation is called when entering the indexInvocation production.
	EnterIndexInvocation(c *IndexInvocationContext)

	// EnterTotalInvocation is called when entering the totalInvocation production.
	EnterTotalInvocation(c *TotalInvocationContext)

	// EnterFunction is called when entering the function production.
	EnterFunction(c *FunctionContext)

	// EnterParamList is called when entering the paramList production.
	EnterParamList(c *ParamListContext)

	// EnterQuantity is called when entering the quantity production.
	EnterQuantity(c *QuantityContext)

	// EnterUnit is called when entering the unit production.
	EnterUnit(c *UnitContext)

	// EnterDateTimePrecision is called when entering the dateTimePrecision production.
	EnterDateTimePrecision(c *DateTimePrecisionContext)

	// EnterPluralDateTimePrecision is called when entering the pluralDateTimePrecision production.
	EnterPluralDateTimePrecision(c *PluralDateTimePrecisionContext)

	// EnterTypeSpecifier is called when entering the typeSpecifier production.
	EnterTypeSpecifier(c *TypeSpecifierContext)

	// EnterQualifiedIdentifier is called when entering the qualifiedIdentifier production.
	EnterQualifiedIdentifier(c *QualifiedIdentifierContext)

	// EnterIdentifier is called when entering the identifier production.
	EnterIdentifier(c *IdentifierContext)

	// ExitIndexerExpression is called when exiting the indexerExpression production.
	ExitIndexerExpression(c *IndexerExpressionContext)

	// ExitPolarityExpression is called when exiting the polarityExpression production.
	ExitPolarityExpression(c *PolarityExpressionContext)

	// ExitAdditiveExpression is called when exiting the additiveExpression production.
	ExitAdditiveExpression(c *AdditiveExpressionContext)

	// ExitMultiplicativeExpression is called when exiting the multiplicativeExpression production.
	ExitMultiplicativeExpression(c *MultiplicativeExpressionContext)

	// ExitUnionExpression is called when exiting the unionExpression production.
	ExitUnionExpression(c *UnionExpressionContext)

	// ExitOrExpression is called when exiting the orExpression production.
	ExitOrExpression(c *OrExpressionContext)

	// ExitAndExpression is called when exiting the andExpression production.
	ExitAndExpression(c *AndExpressionContext)

	// ExitMembershipExpression is called when exiting the membershipExpression production.
	ExitMembershipExpression(c *MembershipExpressionContext)

	// ExitInequalityExpression is called when exiting the inequalityExpression production.
	ExitInequalityExpression(c *InequalityExpressionContext)

	// ExitInvocationExpression is called when exiting the invocationExpression production.
	ExitInvocationExpression(c *InvocationExpressionContext)

	// ExitEqualityExpression is called when exiting the equalityExpression production.
	ExitEqualityExpression(c *EqualityExpressionContext)

	// ExitImpliesExpression is called when exiting the impliesExpression production.
	ExitImpliesExpression(c *ImpliesExpressionContext)

	// ExitTermExpression is called when exiting the termExpression production.
	ExitTermExpression(c *TermExpressionContext)

	// ExitTypeExpression is called when exiting the typeExpression production.
	ExitTypeExpression(c *TypeExpressionContext)

	// ExitInvocationTerm is called when exiting the invocationTerm production.
	ExitInvocationTerm(c *InvocationTermContext)

	// ExitLiteralTerm is called when exiting the literalTerm production.
	ExitLiteralTerm(c *LiteralTermContext)

	// ExitExternalConstantTerm is called when exiting the externalConstantTerm production.
	ExitExternalConstantTerm(c *ExternalConstantTermContext)

	// ExitParenthesizedTerm is called when exiting the parenthesizedTerm production.
	ExitParenthesizedTerm(c *ParenthesizedTermContext)

	// ExitNullLiteral is called when exiting the nullLiteral production.
	ExitNullLiteral(c *NullLiteralContext)

	// ExitBooleanLiteral is called when exiting the booleanLiteral production.
	ExitBooleanLiteral(c *BooleanLiteralContext)

	// ExitStringLiteral is called when exiting the stringLiteral production.
	ExitStringLiteral(c *StringLiteralContext)

	// ExitNumberLiteral is called when exiting the numberLiteral production.
	ExitNumberLiteral(c *NumberLiteralContext)

	// ExitDateLiteral is called when exiting the dateLiteral production.
	ExitDateLiteral(c *DateLiteralContext)

	// ExitDateTimeLiteral is called when exiting the dateTimeLiteral production.
	ExitDateTimeLiteral(c *DateTimeLiteralContext)

	// ExitTimeLiteral is called when exiting the timeLiteral production.
	ExitTimeLiteral(c *TimeLiteralContext)

	// ExitQuantityLiteral is called when exiting the quantityLiteral production.
	ExitQuantityLiteral(c *QuantityLiteralContext)

	// ExitExternalConstant is called when exiting the externalConstant production.
	ExitExternalConstant(c *ExternalConstantContext)

	// ExitMemberInvocation is called when exiting the memberInvocation production.
	ExitMemberInvocation(c *MemberInvocationContext)

	// ExitFunctionInvocation is called when exiting the functionInvocation production.
	ExitFunctionInvocation(c *FunctionInvocationContext)

	// ExitThisInvocation is called when exiting the thisInvocation production.
	ExitThisInvocation(c *ThisInvocationContext)

	// ExitIndexInvocation is called when exiting the indexInvocation production.
	ExitIndexInvocation(c *IndexInvocationContext)

	// ExitTotalInvocation is called when exiting the totalInvocation production.
	ExitTotalInvocation(c *TotalInvocationContext)

	// ExitFunction is called when exiting the function production.
	ExitFunction(c *FunctionContext)

	// ExitParamList is called when exiting the paramList production.
	ExitParamList(c *ParamListContext)

	// ExitQuantity is called when exiting the quantity production.
	ExitQuantity(c *QuantityContext)

	// ExitUnit is called when exiting the unit production.
	ExitUnit(c *UnitContext)

	// ExitDateTimePrecision is called when exiting the dateTimePrecision production.
	ExitDateTimePrecision(c *DateTimePrecisionContext)

	// ExitPluralDateTimePrecision is called when exiting the pluralDateTimePrecision production.
	ExitPluralDateTimePrecision(c *PluralDateTimePrecisionContext)

	// ExitTypeSpecifier is called when exiting the typeSpecifier production.
	ExitTypeSpecifier(c *TypeSpecifierContext)

	// ExitQualifiedIdentifier is called when exiting the qualifiedIdentifier production.
	ExitQualifiedIdentifier(c *QualifiedIdentifierContext)

	// ExitIdentifier is called when exiting the identifier production.
	ExitIdentifier(c *IdentifierContext)
}
