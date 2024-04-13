// Code generated from FhirPath.g4 by ANTLR 4.13.1. DO NOT EDIT.

package gen // FhirPath
import "github.com/antlr4-go/antlr/v4"

// A complete Visitor for a parse tree produced by FhirPathParser.
type FhirPathVisitor interface {
	antlr.ParseTreeVisitor

	// Visit a parse tree produced by FhirPathParser#indexerExpression.
	VisitIndexerExpression(ctx *IndexerExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#polarityExpression.
	VisitPolarityExpression(ctx *PolarityExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#additiveExpression.
	VisitAdditiveExpression(ctx *AdditiveExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#multiplicativeExpression.
	VisitMultiplicativeExpression(ctx *MultiplicativeExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#unionExpression.
	VisitUnionExpression(ctx *UnionExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#orExpression.
	VisitOrExpression(ctx *OrExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#andExpression.
	VisitAndExpression(ctx *AndExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#membershipExpression.
	VisitMembershipExpression(ctx *MembershipExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#inequalityExpression.
	VisitInequalityExpression(ctx *InequalityExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#invocationExpression.
	VisitInvocationExpression(ctx *InvocationExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#equalityExpression.
	VisitEqualityExpression(ctx *EqualityExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#impliesExpression.
	VisitImpliesExpression(ctx *ImpliesExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#termExpression.
	VisitTermExpression(ctx *TermExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#typeExpression.
	VisitTypeExpression(ctx *TypeExpressionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#invocationTerm.
	VisitInvocationTerm(ctx *InvocationTermContext) interface{}

	// Visit a parse tree produced by FhirPathParser#literalTerm.
	VisitLiteralTerm(ctx *LiteralTermContext) interface{}

	// Visit a parse tree produced by FhirPathParser#externalConstantTerm.
	VisitExternalConstantTerm(ctx *ExternalConstantTermContext) interface{}

	// Visit a parse tree produced by FhirPathParser#parenthesizedTerm.
	VisitParenthesizedTerm(ctx *ParenthesizedTermContext) interface{}

	// Visit a parse tree produced by FhirPathParser#nullLiteral.
	VisitNullLiteral(ctx *NullLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#booleanLiteral.
	VisitBooleanLiteral(ctx *BooleanLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#stringLiteral.
	VisitStringLiteral(ctx *StringLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#numberLiteral.
	VisitNumberLiteral(ctx *NumberLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#dateLiteral.
	VisitDateLiteral(ctx *DateLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#dateTimeLiteral.
	VisitDateTimeLiteral(ctx *DateTimeLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#timeLiteral.
	VisitTimeLiteral(ctx *TimeLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#quantityLiteral.
	VisitQuantityLiteral(ctx *QuantityLiteralContext) interface{}

	// Visit a parse tree produced by FhirPathParser#externalConstant.
	VisitExternalConstant(ctx *ExternalConstantContext) interface{}

	// Visit a parse tree produced by FhirPathParser#memberInvocation.
	VisitMemberInvocation(ctx *MemberInvocationContext) interface{}

	// Visit a parse tree produced by FhirPathParser#functionInvocation.
	VisitFunctionInvocation(ctx *FunctionInvocationContext) interface{}

	// Visit a parse tree produced by FhirPathParser#thisInvocation.
	VisitThisInvocation(ctx *ThisInvocationContext) interface{}

	// Visit a parse tree produced by FhirPathParser#indexInvocation.
	VisitIndexInvocation(ctx *IndexInvocationContext) interface{}

	// Visit a parse tree produced by FhirPathParser#totalInvocation.
	VisitTotalInvocation(ctx *TotalInvocationContext) interface{}

	// Visit a parse tree produced by FhirPathParser#function.
	VisitFunction(ctx *FunctionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#paramList.
	VisitParamList(ctx *ParamListContext) interface{}

	// Visit a parse tree produced by FhirPathParser#quantity.
	VisitQuantity(ctx *QuantityContext) interface{}

	// Visit a parse tree produced by FhirPathParser#unit.
	VisitUnit(ctx *UnitContext) interface{}

	// Visit a parse tree produced by FhirPathParser#dateTimePrecision.
	VisitDateTimePrecision(ctx *DateTimePrecisionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#pluralDateTimePrecision.
	VisitPluralDateTimePrecision(ctx *PluralDateTimePrecisionContext) interface{}

	// Visit a parse tree produced by FhirPathParser#typeSpecifier.
	VisitTypeSpecifier(ctx *TypeSpecifierContext) interface{}

	// Visit a parse tree produced by FhirPathParser#qualifiedIdentifier.
	VisitQualifiedIdentifier(ctx *QualifiedIdentifierContext) interface{}

	// Visit a parse tree produced by FhirPathParser#identifier.
	VisitIdentifier(ctx *IdentifierContext) interface{}
}
