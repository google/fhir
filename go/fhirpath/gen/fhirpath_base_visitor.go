// Code generated from FhirPath.g4 by ANTLR 4.13.1. DO NOT EDIT.

package gen // FhirPath
import "github.com/antlr4-go/antlr/v4"

type BaseFhirPathVisitor struct {
	*antlr.BaseParseTreeVisitor
}

func (v *BaseFhirPathVisitor) VisitIndexerExpression(ctx *IndexerExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitPolarityExpression(ctx *PolarityExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitAdditiveExpression(ctx *AdditiveExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitMultiplicativeExpression(ctx *MultiplicativeExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitUnionExpression(ctx *UnionExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitOrExpression(ctx *OrExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitAndExpression(ctx *AndExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitMembershipExpression(ctx *MembershipExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitInequalityExpression(ctx *InequalityExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitInvocationExpression(ctx *InvocationExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitEqualityExpression(ctx *EqualityExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitImpliesExpression(ctx *ImpliesExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitTermExpression(ctx *TermExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitTypeExpression(ctx *TypeExpressionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitInvocationTerm(ctx *InvocationTermContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitLiteralTerm(ctx *LiteralTermContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitExternalConstantTerm(ctx *ExternalConstantTermContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitParenthesizedTerm(ctx *ParenthesizedTermContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitNullLiteral(ctx *NullLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitBooleanLiteral(ctx *BooleanLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitStringLiteral(ctx *StringLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitNumberLiteral(ctx *NumberLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitDateLiteral(ctx *DateLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitDateTimeLiteral(ctx *DateTimeLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitTimeLiteral(ctx *TimeLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitQuantityLiteral(ctx *QuantityLiteralContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitExternalConstant(ctx *ExternalConstantContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitMemberInvocation(ctx *MemberInvocationContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitFunctionInvocation(ctx *FunctionInvocationContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitThisInvocation(ctx *ThisInvocationContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitIndexInvocation(ctx *IndexInvocationContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitTotalInvocation(ctx *TotalInvocationContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitFunction(ctx *FunctionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitParamList(ctx *ParamListContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitQuantity(ctx *QuantityContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitUnit(ctx *UnitContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitDateTimePrecision(ctx *DateTimePrecisionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitPluralDateTimePrecision(ctx *PluralDateTimePrecisionContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitTypeSpecifier(ctx *TypeSpecifierContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitQualifiedIdentifier(ctx *QualifiedIdentifierContext) interface{} {
	return v.VisitChildren(ctx)
}

func (v *BaseFhirPathVisitor) VisitIdentifier(ctx *IdentifierContext) interface{} {
	return v.VisitChildren(ctx)
}
