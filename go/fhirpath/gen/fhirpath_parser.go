// Code generated from FhirPath.g4 by ANTLR 4.13.1. DO NOT EDIT.

package gen // FhirPath
import (
	"fmt"
	"strconv"
	"sync"

	"github.com/antlr4-go/antlr/v4"
)

// Suppress unused import errors
var _ = fmt.Printf
var _ = strconv.Itoa
var _ = sync.Once{}

type FhirPathParser struct {
	*antlr.BaseParser
}

var FhirPathParserStaticData struct {
	once                   sync.Once
	serializedATN          []int32
	LiteralNames           []string
	SymbolicNames          []string
	RuleNames              []string
	PredictionContextCache *antlr.PredictionContextCache
	atn                    *antlr.ATN
	decisionToDFA          []*antlr.DFA
}

func fhirpathParserInit() {
	staticData := &FhirPathParserStaticData
	staticData.LiteralNames = []string{
		"", "'.'", "'['", "']'", "'+'", "'-'", "'*'", "'/'", "'div'", "'mod'",
		"'&'", "'is'", "'as'", "'|'", "'<='", "'<'", "'>'", "'>='", "'='", "'~'",
		"'!='", "'!~'", "'in'", "'contains'", "'and'", "'or'", "'xor'", "'implies'",
		"'('", "')'", "'{'", "'}'", "'true'", "'false'", "'%'", "'$this'", "'$index'",
		"'$total'", "','", "'year'", "'month'", "'week'", "'day'", "'hour'",
		"'minute'", "'second'", "'millisecond'", "'years'", "'months'", "'weeks'",
		"'days'", "'hours'", "'minutes'", "'seconds'", "'milliseconds'",
	}
	staticData.SymbolicNames = []string{
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "DATE", "DATETIME", "TIME", "IDENTIFIER", "DELIMITEDIDENTIFIER",
		"STRING", "NUMBER", "WS", "COMMENT", "LINE_COMMENT",
	}
	staticData.RuleNames = []string{
		"expression", "term", "literal", "externalConstant", "invocation", "function",
		"paramList", "quantity", "unit", "dateTimePrecision", "pluralDateTimePrecision",
		"typeSpecifier", "qualifiedIdentifier", "identifier",
	}
	staticData.PredictionContextCache = antlr.NewPredictionContextCache()
	staticData.serializedATN = []int32{
		4, 1, 64, 150, 2, 0, 7, 0, 2, 1, 7, 1, 2, 2, 7, 2, 2, 3, 7, 3, 2, 4, 7,
		4, 2, 5, 7, 5, 2, 6, 7, 6, 2, 7, 7, 7, 2, 8, 7, 8, 2, 9, 7, 9, 2, 10, 7,
		10, 2, 11, 7, 11, 2, 12, 7, 12, 2, 13, 7, 13, 1, 0, 1, 0, 1, 0, 1, 0, 3,
		0, 33, 8, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
		1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
		1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
		1, 0, 1, 0, 1, 0, 1, 0, 5, 0, 73, 8, 0, 10, 0, 12, 0, 76, 9, 0, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 85, 8, 1, 1, 2, 1, 2, 1, 2, 1, 2,
		1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 3, 2, 96, 8, 2, 1, 3, 1, 3, 1, 3, 3, 3, 101,
		8, 3, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 3, 4, 108, 8, 4, 1, 5, 1, 5, 1, 5,
		3, 5, 113, 8, 5, 1, 5, 1, 5, 1, 6, 1, 6, 1, 6, 5, 6, 120, 8, 6, 10, 6,
		12, 6, 123, 9, 6, 1, 7, 1, 7, 3, 7, 127, 8, 7, 1, 8, 1, 8, 1, 8, 3, 8,
		132, 8, 8, 1, 9, 1, 9, 1, 10, 1, 10, 1, 11, 1, 11, 1, 12, 1, 12, 1, 12,
		5, 12, 143, 8, 12, 10, 12, 12, 12, 146, 9, 12, 1, 13, 1, 13, 1, 13, 0,
		1, 0, 14, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 0, 12, 1,
		0, 4, 5, 1, 0, 6, 9, 2, 0, 4, 5, 10, 10, 1, 0, 14, 17, 1, 0, 18, 21, 1,
		0, 22, 23, 1, 0, 25, 26, 1, 0, 11, 12, 1, 0, 32, 33, 1, 0, 39, 46, 1, 0,
		47, 54, 3, 0, 11, 12, 22, 23, 58, 59, 169, 0, 32, 1, 0, 0, 0, 2, 84, 1,
		0, 0, 0, 4, 95, 1, 0, 0, 0, 6, 97, 1, 0, 0, 0, 8, 107, 1, 0, 0, 0, 10,
		109, 1, 0, 0, 0, 12, 116, 1, 0, 0, 0, 14, 124, 1, 0, 0, 0, 16, 131, 1,
		0, 0, 0, 18, 133, 1, 0, 0, 0, 20, 135, 1, 0, 0, 0, 22, 137, 1, 0, 0, 0,
		24, 139, 1, 0, 0, 0, 26, 147, 1, 0, 0, 0, 28, 29, 6, 0, -1, 0, 29, 33,
		3, 2, 1, 0, 30, 31, 7, 0, 0, 0, 31, 33, 3, 0, 0, 11, 32, 28, 1, 0, 0, 0,
		32, 30, 1, 0, 0, 0, 33, 74, 1, 0, 0, 0, 34, 35, 10, 10, 0, 0, 35, 36, 7,
		1, 0, 0, 36, 73, 3, 0, 0, 11, 37, 38, 10, 9, 0, 0, 38, 39, 7, 2, 0, 0,
		39, 73, 3, 0, 0, 10, 40, 41, 10, 7, 0, 0, 41, 42, 5, 13, 0, 0, 42, 73,
		3, 0, 0, 8, 43, 44, 10, 6, 0, 0, 44, 45, 7, 3, 0, 0, 45, 73, 3, 0, 0, 7,
		46, 47, 10, 5, 0, 0, 47, 48, 7, 4, 0, 0, 48, 73, 3, 0, 0, 6, 49, 50, 10,
		4, 0, 0, 50, 51, 7, 5, 0, 0, 51, 73, 3, 0, 0, 5, 52, 53, 10, 3, 0, 0, 53,
		54, 5, 24, 0, 0, 54, 73, 3, 0, 0, 4, 55, 56, 10, 2, 0, 0, 56, 57, 7, 6,
		0, 0, 57, 73, 3, 0, 0, 3, 58, 59, 10, 1, 0, 0, 59, 60, 5, 27, 0, 0, 60,
		73, 3, 0, 0, 2, 61, 62, 10, 13, 0, 0, 62, 63, 5, 1, 0, 0, 63, 73, 3, 8,
		4, 0, 64, 65, 10, 12, 0, 0, 65, 66, 5, 2, 0, 0, 66, 67, 3, 0, 0, 0, 67,
		68, 5, 3, 0, 0, 68, 73, 1, 0, 0, 0, 69, 70, 10, 8, 0, 0, 70, 71, 7, 7,
		0, 0, 71, 73, 3, 22, 11, 0, 72, 34, 1, 0, 0, 0, 72, 37, 1, 0, 0, 0, 72,
		40, 1, 0, 0, 0, 72, 43, 1, 0, 0, 0, 72, 46, 1, 0, 0, 0, 72, 49, 1, 0, 0,
		0, 72, 52, 1, 0, 0, 0, 72, 55, 1, 0, 0, 0, 72, 58, 1, 0, 0, 0, 72, 61,
		1, 0, 0, 0, 72, 64, 1, 0, 0, 0, 72, 69, 1, 0, 0, 0, 73, 76, 1, 0, 0, 0,
		74, 72, 1, 0, 0, 0, 74, 75, 1, 0, 0, 0, 75, 1, 1, 0, 0, 0, 76, 74, 1, 0,
		0, 0, 77, 85, 3, 8, 4, 0, 78, 85, 3, 4, 2, 0, 79, 85, 3, 6, 3, 0, 80, 81,
		5, 28, 0, 0, 81, 82, 3, 0, 0, 0, 82, 83, 5, 29, 0, 0, 83, 85, 1, 0, 0,
		0, 84, 77, 1, 0, 0, 0, 84, 78, 1, 0, 0, 0, 84, 79, 1, 0, 0, 0, 84, 80,
		1, 0, 0, 0, 85, 3, 1, 0, 0, 0, 86, 87, 5, 30, 0, 0, 87, 96, 5, 31, 0, 0,
		88, 96, 7, 8, 0, 0, 89, 96, 5, 60, 0, 0, 90, 96, 5, 61, 0, 0, 91, 96, 5,
		55, 0, 0, 92, 96, 5, 56, 0, 0, 93, 96, 5, 57, 0, 0, 94, 96, 3, 14, 7, 0,
		95, 86, 1, 0, 0, 0, 95, 88, 1, 0, 0, 0, 95, 89, 1, 0, 0, 0, 95, 90, 1,
		0, 0, 0, 95, 91, 1, 0, 0, 0, 95, 92, 1, 0, 0, 0, 95, 93, 1, 0, 0, 0, 95,
		94, 1, 0, 0, 0, 96, 5, 1, 0, 0, 0, 97, 100, 5, 34, 0, 0, 98, 101, 3, 26,
		13, 0, 99, 101, 5, 60, 0, 0, 100, 98, 1, 0, 0, 0, 100, 99, 1, 0, 0, 0,
		101, 7, 1, 0, 0, 0, 102, 108, 3, 26, 13, 0, 103, 108, 3, 10, 5, 0, 104,
		108, 5, 35, 0, 0, 105, 108, 5, 36, 0, 0, 106, 108, 5, 37, 0, 0, 107, 102,
		1, 0, 0, 0, 107, 103, 1, 0, 0, 0, 107, 104, 1, 0, 0, 0, 107, 105, 1, 0,
		0, 0, 107, 106, 1, 0, 0, 0, 108, 9, 1, 0, 0, 0, 109, 110, 3, 26, 13, 0,
		110, 112, 5, 28, 0, 0, 111, 113, 3, 12, 6, 0, 112, 111, 1, 0, 0, 0, 112,
		113, 1, 0, 0, 0, 113, 114, 1, 0, 0, 0, 114, 115, 5, 29, 0, 0, 115, 11,
		1, 0, 0, 0, 116, 121, 3, 0, 0, 0, 117, 118, 5, 38, 0, 0, 118, 120, 3, 0,
		0, 0, 119, 117, 1, 0, 0, 0, 120, 123, 1, 0, 0, 0, 121, 119, 1, 0, 0, 0,
		121, 122, 1, 0, 0, 0, 122, 13, 1, 0, 0, 0, 123, 121, 1, 0, 0, 0, 124, 126,
		5, 61, 0, 0, 125, 127, 3, 16, 8, 0, 126, 125, 1, 0, 0, 0, 126, 127, 1,
		0, 0, 0, 127, 15, 1, 0, 0, 0, 128, 132, 3, 18, 9, 0, 129, 132, 3, 20, 10,
		0, 130, 132, 5, 60, 0, 0, 131, 128, 1, 0, 0, 0, 131, 129, 1, 0, 0, 0, 131,
		130, 1, 0, 0, 0, 132, 17, 1, 0, 0, 0, 133, 134, 7, 9, 0, 0, 134, 19, 1,
		0, 0, 0, 135, 136, 7, 10, 0, 0, 136, 21, 1, 0, 0, 0, 137, 138, 3, 24, 12,
		0, 138, 23, 1, 0, 0, 0, 139, 144, 3, 26, 13, 0, 140, 141, 5, 1, 0, 0, 141,
		143, 3, 26, 13, 0, 142, 140, 1, 0, 0, 0, 143, 146, 1, 0, 0, 0, 144, 142,
		1, 0, 0, 0, 144, 145, 1, 0, 0, 0, 145, 25, 1, 0, 0, 0, 146, 144, 1, 0,
		0, 0, 147, 148, 7, 11, 0, 0, 148, 27, 1, 0, 0, 0, 12, 32, 72, 74, 84, 95,
		100, 107, 112, 121, 126, 131, 144,
	}
	deserializer := antlr.NewATNDeserializer(nil)
	staticData.atn = deserializer.Deserialize(staticData.serializedATN)
	atn := staticData.atn
	staticData.decisionToDFA = make([]*antlr.DFA, len(atn.DecisionToState))
	decisionToDFA := staticData.decisionToDFA
	for index, state := range atn.DecisionToState {
		decisionToDFA[index] = antlr.NewDFA(state, index)
	}
}

// FhirPathParserInit initializes any static state used to implement FhirPathParser. By default the
// static state used to implement the parser is lazily initialized during the first call to
// NewFhirPathParser(). You can call this function if you wish to initialize the static state ahead
// of time.
func FhirPathParserInit() {
	staticData := &FhirPathParserStaticData
	staticData.once.Do(fhirpathParserInit)
}

// NewFhirPathParser produces a new parser instance for the optional input antlr.TokenStream.
func NewFhirPathParser(input antlr.TokenStream) *FhirPathParser {
	FhirPathParserInit()
	this := new(FhirPathParser)
	this.BaseParser = antlr.NewBaseParser(input)
	staticData := &FhirPathParserStaticData
	this.Interpreter = antlr.NewParserATNSimulator(this, staticData.atn, staticData.decisionToDFA, staticData.PredictionContextCache)
	this.RuleNames = staticData.RuleNames
	this.LiteralNames = staticData.LiteralNames
	this.SymbolicNames = staticData.SymbolicNames
	this.GrammarFileName = "FhirPath.g4"

	return this
}

// FhirPathParser tokens.
const (
	FhirPathParserEOF                 = antlr.TokenEOF
	FhirPathParserT__0                = 1
	FhirPathParserT__1                = 2
	FhirPathParserT__2                = 3
	FhirPathParserT__3                = 4
	FhirPathParserT__4                = 5
	FhirPathParserT__5                = 6
	FhirPathParserT__6                = 7
	FhirPathParserT__7                = 8
	FhirPathParserT__8                = 9
	FhirPathParserT__9                = 10
	FhirPathParserT__10               = 11
	FhirPathParserT__11               = 12
	FhirPathParserT__12               = 13
	FhirPathParserT__13               = 14
	FhirPathParserT__14               = 15
	FhirPathParserT__15               = 16
	FhirPathParserT__16               = 17
	FhirPathParserT__17               = 18
	FhirPathParserT__18               = 19
	FhirPathParserT__19               = 20
	FhirPathParserT__20               = 21
	FhirPathParserT__21               = 22
	FhirPathParserT__22               = 23
	FhirPathParserT__23               = 24
	FhirPathParserT__24               = 25
	FhirPathParserT__25               = 26
	FhirPathParserT__26               = 27
	FhirPathParserT__27               = 28
	FhirPathParserT__28               = 29
	FhirPathParserT__29               = 30
	FhirPathParserT__30               = 31
	FhirPathParserT__31               = 32
	FhirPathParserT__32               = 33
	FhirPathParserT__33               = 34
	FhirPathParserT__34               = 35
	FhirPathParserT__35               = 36
	FhirPathParserT__36               = 37
	FhirPathParserT__37               = 38
	FhirPathParserT__38               = 39
	FhirPathParserT__39               = 40
	FhirPathParserT__40               = 41
	FhirPathParserT__41               = 42
	FhirPathParserT__42               = 43
	FhirPathParserT__43               = 44
	FhirPathParserT__44               = 45
	FhirPathParserT__45               = 46
	FhirPathParserT__46               = 47
	FhirPathParserT__47               = 48
	FhirPathParserT__48               = 49
	FhirPathParserT__49               = 50
	FhirPathParserT__50               = 51
	FhirPathParserT__51               = 52
	FhirPathParserT__52               = 53
	FhirPathParserT__53               = 54
	FhirPathParserDATE                = 55
	FhirPathParserDATETIME            = 56
	FhirPathParserTIME                = 57
	FhirPathParserIDENTIFIER          = 58
	FhirPathParserDELIMITEDIDENTIFIER = 59
	FhirPathParserSTRING              = 60
	FhirPathParserNUMBER              = 61
	FhirPathParserWS                  = 62
	FhirPathParserCOMMENT             = 63
	FhirPathParserLINE_COMMENT        = 64
)

// FhirPathParser rules.
const (
	FhirPathParserRULE_expression              = 0
	FhirPathParserRULE_term                    = 1
	FhirPathParserRULE_literal                 = 2
	FhirPathParserRULE_externalConstant        = 3
	FhirPathParserRULE_invocation              = 4
	FhirPathParserRULE_function                = 5
	FhirPathParserRULE_paramList               = 6
	FhirPathParserRULE_quantity                = 7
	FhirPathParserRULE_unit                    = 8
	FhirPathParserRULE_dateTimePrecision       = 9
	FhirPathParserRULE_pluralDateTimePrecision = 10
	FhirPathParserRULE_typeSpecifier           = 11
	FhirPathParserRULE_qualifiedIdentifier     = 12
	FhirPathParserRULE_identifier              = 13
)

// IExpressionContext is an interface to support dynamic dispatch.
type IExpressionContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser
	// IsExpressionContext differentiates from other interfaces.
	IsExpressionContext()
}

type ExpressionContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyExpressionContext() *ExpressionContext {
	var p = new(ExpressionContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_expression
	return p
}

func InitEmptyExpressionContext(p *ExpressionContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_expression
}

func (*ExpressionContext) IsExpressionContext() {}

func NewExpressionContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *ExpressionContext {
	var p = new(ExpressionContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_expression

	return p
}

func (s *ExpressionContext) GetParser() antlr.Parser { return s.parser }

func (s *ExpressionContext) CopyAll(ctx *ExpressionContext) {
	s.CopyFrom(&ctx.BaseParserRuleContext)
}

func (s *ExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ExpressionContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

type IndexerExpressionContext struct {
	ExpressionContext
}

func NewIndexerExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *IndexerExpressionContext {
	var p = new(IndexerExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *IndexerExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *IndexerExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *IndexerExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *IndexerExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterIndexerExpression(s)
	}
}

func (s *IndexerExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitIndexerExpression(s)
	}
}

func (s *IndexerExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitIndexerExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type PolarityExpressionContext struct {
	ExpressionContext
}

func NewPolarityExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *PolarityExpressionContext {
	var p = new(PolarityExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *PolarityExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *PolarityExpressionContext) Expression() IExpressionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *PolarityExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterPolarityExpression(s)
	}
}

func (s *PolarityExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitPolarityExpression(s)
	}
}

func (s *PolarityExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitPolarityExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type AdditiveExpressionContext struct {
	ExpressionContext
}

func NewAdditiveExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *AdditiveExpressionContext {
	var p = new(AdditiveExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *AdditiveExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *AdditiveExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *AdditiveExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *AdditiveExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterAdditiveExpression(s)
	}
}

func (s *AdditiveExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitAdditiveExpression(s)
	}
}

func (s *AdditiveExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitAdditiveExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type MultiplicativeExpressionContext struct {
	ExpressionContext
}

func NewMultiplicativeExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *MultiplicativeExpressionContext {
	var p = new(MultiplicativeExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *MultiplicativeExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *MultiplicativeExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *MultiplicativeExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *MultiplicativeExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterMultiplicativeExpression(s)
	}
}

func (s *MultiplicativeExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitMultiplicativeExpression(s)
	}
}

func (s *MultiplicativeExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitMultiplicativeExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type UnionExpressionContext struct {
	ExpressionContext
}

func NewUnionExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *UnionExpressionContext {
	var p = new(UnionExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *UnionExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *UnionExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *UnionExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *UnionExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterUnionExpression(s)
	}
}

func (s *UnionExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitUnionExpression(s)
	}
}

func (s *UnionExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitUnionExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type OrExpressionContext struct {
	ExpressionContext
}

func NewOrExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *OrExpressionContext {
	var p = new(OrExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *OrExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *OrExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *OrExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *OrExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterOrExpression(s)
	}
}

func (s *OrExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitOrExpression(s)
	}
}

func (s *OrExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitOrExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type AndExpressionContext struct {
	ExpressionContext
}

func NewAndExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *AndExpressionContext {
	var p = new(AndExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *AndExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *AndExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *AndExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *AndExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterAndExpression(s)
	}
}

func (s *AndExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitAndExpression(s)
	}
}

func (s *AndExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitAndExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type MembershipExpressionContext struct {
	ExpressionContext
}

func NewMembershipExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *MembershipExpressionContext {
	var p = new(MembershipExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *MembershipExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *MembershipExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *MembershipExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *MembershipExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterMembershipExpression(s)
	}
}

func (s *MembershipExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitMembershipExpression(s)
	}
}

func (s *MembershipExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitMembershipExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type InequalityExpressionContext struct {
	ExpressionContext
}

func NewInequalityExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *InequalityExpressionContext {
	var p = new(InequalityExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *InequalityExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *InequalityExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *InequalityExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *InequalityExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterInequalityExpression(s)
	}
}

func (s *InequalityExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitInequalityExpression(s)
	}
}

func (s *InequalityExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitInequalityExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type InvocationExpressionContext struct {
	ExpressionContext
}

func NewInvocationExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *InvocationExpressionContext {
	var p = new(InvocationExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *InvocationExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *InvocationExpressionContext) Expression() IExpressionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *InvocationExpressionContext) Invocation() IInvocationContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IInvocationContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IInvocationContext)
}

func (s *InvocationExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterInvocationExpression(s)
	}
}

func (s *InvocationExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitInvocationExpression(s)
	}
}

func (s *InvocationExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitInvocationExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type EqualityExpressionContext struct {
	ExpressionContext
}

func NewEqualityExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *EqualityExpressionContext {
	var p = new(EqualityExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *EqualityExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *EqualityExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *EqualityExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *EqualityExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterEqualityExpression(s)
	}
}

func (s *EqualityExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitEqualityExpression(s)
	}
}

func (s *EqualityExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitEqualityExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type ImpliesExpressionContext struct {
	ExpressionContext
}

func NewImpliesExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *ImpliesExpressionContext {
	var p = new(ImpliesExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *ImpliesExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ImpliesExpressionContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *ImpliesExpressionContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *ImpliesExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterImpliesExpression(s)
	}
}

func (s *ImpliesExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitImpliesExpression(s)
	}
}

func (s *ImpliesExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitImpliesExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type TermExpressionContext struct {
	ExpressionContext
}

func NewTermExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *TermExpressionContext {
	var p = new(TermExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *TermExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *TermExpressionContext) Term() ITermContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(ITermContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(ITermContext)
}

func (s *TermExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterTermExpression(s)
	}
}

func (s *TermExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitTermExpression(s)
	}
}

func (s *TermExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitTermExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

type TypeExpressionContext struct {
	ExpressionContext
}

func NewTypeExpressionContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *TypeExpressionContext {
	var p = new(TypeExpressionContext)

	InitEmptyExpressionContext(&p.ExpressionContext)
	p.parser = parser
	p.CopyAll(ctx.(*ExpressionContext))

	return p
}

func (s *TypeExpressionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *TypeExpressionContext) Expression() IExpressionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *TypeExpressionContext) TypeSpecifier() ITypeSpecifierContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(ITypeSpecifierContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(ITypeSpecifierContext)
}

func (s *TypeExpressionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterTypeExpression(s)
	}
}

func (s *TypeExpressionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitTypeExpression(s)
	}
}

func (s *TypeExpressionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitTypeExpression(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Expression() (localctx IExpressionContext) {
	return p.expression(0)
}

func (p *FhirPathParser) expression(_p int) (localctx IExpressionContext) {
	var _parentctx antlr.ParserRuleContext = p.GetParserRuleContext()

	_parentState := p.GetState()
	localctx = NewExpressionContext(p, p.GetParserRuleContext(), _parentState)
	var _prevctx IExpressionContext = localctx
	var _ antlr.ParserRuleContext = _prevctx // TODO: To prevent unused variable warning.
	_startState := 0
	p.EnterRecursionRule(localctx, 0, FhirPathParserRULE_expression, _p)
	var _la int

	var _alt int

	p.EnterOuterAlt(localctx, 1)
	p.SetState(32)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}

	switch p.GetTokenStream().LA(1) {
	case FhirPathParserT__10, FhirPathParserT__11, FhirPathParserT__21, FhirPathParserT__22, FhirPathParserT__27, FhirPathParserT__29, FhirPathParserT__31, FhirPathParserT__32, FhirPathParserT__33, FhirPathParserT__34, FhirPathParserT__35, FhirPathParserT__36, FhirPathParserDATE, FhirPathParserDATETIME, FhirPathParserTIME, FhirPathParserIDENTIFIER, FhirPathParserDELIMITEDIDENTIFIER, FhirPathParserSTRING, FhirPathParserNUMBER:
		localctx = NewTermExpressionContext(p, localctx)
		p.SetParserRuleContext(localctx)
		_prevctx = localctx

		{
			p.SetState(29)
			p.Term()
		}

	case FhirPathParserT__3, FhirPathParserT__4:
		localctx = NewPolarityExpressionContext(p, localctx)
		p.SetParserRuleContext(localctx)
		_prevctx = localctx
		{
			p.SetState(30)
			_la = p.GetTokenStream().LA(1)

			if !(_la == FhirPathParserT__3 || _la == FhirPathParserT__4) {
				p.GetErrorHandler().RecoverInline(p)
			} else {
				p.GetErrorHandler().ReportMatch(p)
				p.Consume()
			}
		}
		{
			p.SetState(31)
			p.expression(11)
		}

	default:
		p.SetError(antlr.NewNoViableAltException(p, nil, nil, nil, nil, nil))
		goto errorExit
	}
	p.GetParserRuleContext().SetStop(p.GetTokenStream().LT(-1))
	p.SetState(74)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}
	_alt = p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 2, p.GetParserRuleContext())
	if p.HasError() {
		goto errorExit
	}
	for _alt != 2 && _alt != antlr.ATNInvalidAltNumber {
		if _alt == 1 {
			if p.GetParseListeners() != nil {
				p.TriggerExitRuleEvent()
			}
			_prevctx = localctx
			p.SetState(72)
			p.GetErrorHandler().Sync(p)
			if p.HasError() {
				goto errorExit
			}

			switch p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 1, p.GetParserRuleContext()) {
			case 1:
				localctx = NewMultiplicativeExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(34)

				if !(p.Precpred(p.GetParserRuleContext(), 10)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 10)", ""))
					goto errorExit
				}
				{
					p.SetState(35)
					_la = p.GetTokenStream().LA(1)

					if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&960) != 0) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(36)
					p.expression(11)
				}

			case 2:
				localctx = NewAdditiveExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(37)

				if !(p.Precpred(p.GetParserRuleContext(), 9)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 9)", ""))
					goto errorExit
				}
				{
					p.SetState(38)
					_la = p.GetTokenStream().LA(1)

					if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&1072) != 0) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(39)
					p.expression(10)
				}

			case 3:
				localctx = NewUnionExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(40)

				if !(p.Precpred(p.GetParserRuleContext(), 7)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 7)", ""))
					goto errorExit
				}
				{
					p.SetState(41)
					p.Match(FhirPathParserT__12)
					if p.HasError() {
						// Recognition error - abort rule
						goto errorExit
					}
				}
				{
					p.SetState(42)
					p.expression(8)
				}

			case 4:
				localctx = NewInequalityExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(43)

				if !(p.Precpred(p.GetParserRuleContext(), 6)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 6)", ""))
					goto errorExit
				}
				{
					p.SetState(44)
					_la = p.GetTokenStream().LA(1)

					if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&245760) != 0) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(45)
					p.expression(7)
				}

			case 5:
				localctx = NewEqualityExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(46)

				if !(p.Precpred(p.GetParserRuleContext(), 5)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 5)", ""))
					goto errorExit
				}
				{
					p.SetState(47)
					_la = p.GetTokenStream().LA(1)

					if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&3932160) != 0) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(48)
					p.expression(6)
				}

			case 6:
				localctx = NewMembershipExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(49)

				if !(p.Precpred(p.GetParserRuleContext(), 4)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 4)", ""))
					goto errorExit
				}
				{
					p.SetState(50)
					_la = p.GetTokenStream().LA(1)

					if !(_la == FhirPathParserT__21 || _la == FhirPathParserT__22) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(51)
					p.expression(5)
				}

			case 7:
				localctx = NewAndExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(52)

				if !(p.Precpred(p.GetParserRuleContext(), 3)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 3)", ""))
					goto errorExit
				}
				{
					p.SetState(53)
					p.Match(FhirPathParserT__23)
					if p.HasError() {
						// Recognition error - abort rule
						goto errorExit
					}
				}
				{
					p.SetState(54)
					p.expression(4)
				}

			case 8:
				localctx = NewOrExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(55)

				if !(p.Precpred(p.GetParserRuleContext(), 2)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 2)", ""))
					goto errorExit
				}
				{
					p.SetState(56)
					_la = p.GetTokenStream().LA(1)

					if !(_la == FhirPathParserT__24 || _la == FhirPathParserT__25) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(57)
					p.expression(3)
				}

			case 9:
				localctx = NewImpliesExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(58)

				if !(p.Precpred(p.GetParserRuleContext(), 1)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 1)", ""))
					goto errorExit
				}
				{
					p.SetState(59)
					p.Match(FhirPathParserT__26)
					if p.HasError() {
						// Recognition error - abort rule
						goto errorExit
					}
				}
				{
					p.SetState(60)
					p.expression(2)
				}

			case 10:
				localctx = NewInvocationExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(61)

				if !(p.Precpred(p.GetParserRuleContext(), 13)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 13)", ""))
					goto errorExit
				}
				{
					p.SetState(62)
					p.Match(FhirPathParserT__0)
					if p.HasError() {
						// Recognition error - abort rule
						goto errorExit
					}
				}
				{
					p.SetState(63)
					p.Invocation()
				}

			case 11:
				localctx = NewIndexerExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(64)

				if !(p.Precpred(p.GetParserRuleContext(), 12)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 12)", ""))
					goto errorExit
				}
				{
					p.SetState(65)
					p.Match(FhirPathParserT__1)
					if p.HasError() {
						// Recognition error - abort rule
						goto errorExit
					}
				}
				{
					p.SetState(66)
					p.expression(0)
				}
				{
					p.SetState(67)
					p.Match(FhirPathParserT__2)
					if p.HasError() {
						// Recognition error - abort rule
						goto errorExit
					}
				}

			case 12:
				localctx = NewTypeExpressionContext(p, NewExpressionContext(p, _parentctx, _parentState))
				p.PushNewRecursionContext(localctx, _startState, FhirPathParserRULE_expression)
				p.SetState(69)

				if !(p.Precpred(p.GetParserRuleContext(), 8)) {
					p.SetError(antlr.NewFailedPredicateException(p, "p.Precpred(p.GetParserRuleContext(), 8)", ""))
					goto errorExit
				}
				{
					p.SetState(70)
					_la = p.GetTokenStream().LA(1)

					if !(_la == FhirPathParserT__10 || _la == FhirPathParserT__11) {
						p.GetErrorHandler().RecoverInline(p)
					} else {
						p.GetErrorHandler().ReportMatch(p)
						p.Consume()
					}
				}
				{
					p.SetState(71)
					p.TypeSpecifier()
				}

			case antlr.ATNInvalidAltNumber:
				goto errorExit
			}

		}
		p.SetState(76)
		p.GetErrorHandler().Sync(p)
		if p.HasError() {
			goto errorExit
		}
		_alt = p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 2, p.GetParserRuleContext())
		if p.HasError() {
			goto errorExit
		}
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.UnrollRecursionContexts(_parentctx)
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// ITermContext is an interface to support dynamic dispatch.
type ITermContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser
	// IsTermContext differentiates from other interfaces.
	IsTermContext()
}

type TermContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyTermContext() *TermContext {
	var p = new(TermContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_term
	return p
}

func InitEmptyTermContext(p *TermContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_term
}

func (*TermContext) IsTermContext() {}

func NewTermContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *TermContext {
	var p = new(TermContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_term

	return p
}

func (s *TermContext) GetParser() antlr.Parser { return s.parser }

func (s *TermContext) CopyAll(ctx *TermContext) {
	s.CopyFrom(&ctx.BaseParserRuleContext)
}

func (s *TermContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *TermContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

type ExternalConstantTermContext struct {
	TermContext
}

func NewExternalConstantTermContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *ExternalConstantTermContext {
	var p = new(ExternalConstantTermContext)

	InitEmptyTermContext(&p.TermContext)
	p.parser = parser
	p.CopyAll(ctx.(*TermContext))

	return p
}

func (s *ExternalConstantTermContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ExternalConstantTermContext) ExternalConstant() IExternalConstantContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExternalConstantContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExternalConstantContext)
}

func (s *ExternalConstantTermContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterExternalConstantTerm(s)
	}
}

func (s *ExternalConstantTermContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitExternalConstantTerm(s)
	}
}

func (s *ExternalConstantTermContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitExternalConstantTerm(s)

	default:
		return t.VisitChildren(s)
	}
}

type LiteralTermContext struct {
	TermContext
}

func NewLiteralTermContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *LiteralTermContext {
	var p = new(LiteralTermContext)

	InitEmptyTermContext(&p.TermContext)
	p.parser = parser
	p.CopyAll(ctx.(*TermContext))

	return p
}

func (s *LiteralTermContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *LiteralTermContext) Literal() ILiteralContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(ILiteralContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(ILiteralContext)
}

func (s *LiteralTermContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterLiteralTerm(s)
	}
}

func (s *LiteralTermContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitLiteralTerm(s)
	}
}

func (s *LiteralTermContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitLiteralTerm(s)

	default:
		return t.VisitChildren(s)
	}
}

type ParenthesizedTermContext struct {
	TermContext
}

func NewParenthesizedTermContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *ParenthesizedTermContext {
	var p = new(ParenthesizedTermContext)

	InitEmptyTermContext(&p.TermContext)
	p.parser = parser
	p.CopyAll(ctx.(*TermContext))

	return p
}

func (s *ParenthesizedTermContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ParenthesizedTermContext) Expression() IExpressionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *ParenthesizedTermContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterParenthesizedTerm(s)
	}
}

func (s *ParenthesizedTermContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitParenthesizedTerm(s)
	}
}

func (s *ParenthesizedTermContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitParenthesizedTerm(s)

	default:
		return t.VisitChildren(s)
	}
}

type InvocationTermContext struct {
	TermContext
}

func NewInvocationTermContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *InvocationTermContext {
	var p = new(InvocationTermContext)

	InitEmptyTermContext(&p.TermContext)
	p.parser = parser
	p.CopyAll(ctx.(*TermContext))

	return p
}

func (s *InvocationTermContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *InvocationTermContext) Invocation() IInvocationContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IInvocationContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IInvocationContext)
}

func (s *InvocationTermContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterInvocationTerm(s)
	}
}

func (s *InvocationTermContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitInvocationTerm(s)
	}
}

func (s *InvocationTermContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitInvocationTerm(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Term() (localctx ITermContext) {
	localctx = NewTermContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 2, FhirPathParserRULE_term)
	p.SetState(84)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}

	switch p.GetTokenStream().LA(1) {
	case FhirPathParserT__10, FhirPathParserT__11, FhirPathParserT__21, FhirPathParserT__22, FhirPathParserT__34, FhirPathParserT__35, FhirPathParserT__36, FhirPathParserIDENTIFIER, FhirPathParserDELIMITEDIDENTIFIER:
		localctx = NewInvocationTermContext(p, localctx)
		p.EnterOuterAlt(localctx, 1)
		{
			p.SetState(77)
			p.Invocation()
		}

	case FhirPathParserT__29, FhirPathParserT__31, FhirPathParserT__32, FhirPathParserDATE, FhirPathParserDATETIME, FhirPathParserTIME, FhirPathParserSTRING, FhirPathParserNUMBER:
		localctx = NewLiteralTermContext(p, localctx)
		p.EnterOuterAlt(localctx, 2)
		{
			p.SetState(78)
			p.Literal()
		}

	case FhirPathParserT__33:
		localctx = NewExternalConstantTermContext(p, localctx)
		p.EnterOuterAlt(localctx, 3)
		{
			p.SetState(79)
			p.ExternalConstant()
		}

	case FhirPathParserT__27:
		localctx = NewParenthesizedTermContext(p, localctx)
		p.EnterOuterAlt(localctx, 4)
		{
			p.SetState(80)
			p.Match(FhirPathParserT__27)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}
		{
			p.SetState(81)
			p.expression(0)
		}
		{
			p.SetState(82)
			p.Match(FhirPathParserT__28)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	default:
		p.SetError(antlr.NewNoViableAltException(p, nil, nil, nil, nil, nil))
		goto errorExit
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// ILiteralContext is an interface to support dynamic dispatch.
type ILiteralContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser
	// IsLiteralContext differentiates from other interfaces.
	IsLiteralContext()
}

type LiteralContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyLiteralContext() *LiteralContext {
	var p = new(LiteralContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_literal
	return p
}

func InitEmptyLiteralContext(p *LiteralContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_literal
}

func (*LiteralContext) IsLiteralContext() {}

func NewLiteralContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *LiteralContext {
	var p = new(LiteralContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_literal

	return p
}

func (s *LiteralContext) GetParser() antlr.Parser { return s.parser }

func (s *LiteralContext) CopyAll(ctx *LiteralContext) {
	s.CopyFrom(&ctx.BaseParserRuleContext)
}

func (s *LiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *LiteralContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

type TimeLiteralContext struct {
	LiteralContext
}

func NewTimeLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *TimeLiteralContext {
	var p = new(TimeLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *TimeLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *TimeLiteralContext) TIME() antlr.TerminalNode {
	return s.GetToken(FhirPathParserTIME, 0)
}

func (s *TimeLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterTimeLiteral(s)
	}
}

func (s *TimeLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitTimeLiteral(s)
	}
}

func (s *TimeLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitTimeLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type NullLiteralContext struct {
	LiteralContext
}

func NewNullLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *NullLiteralContext {
	var p = new(NullLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *NullLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *NullLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterNullLiteral(s)
	}
}

func (s *NullLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitNullLiteral(s)
	}
}

func (s *NullLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitNullLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type DateTimeLiteralContext struct {
	LiteralContext
}

func NewDateTimeLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *DateTimeLiteralContext {
	var p = new(DateTimeLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *DateTimeLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *DateTimeLiteralContext) DATETIME() antlr.TerminalNode {
	return s.GetToken(FhirPathParserDATETIME, 0)
}

func (s *DateTimeLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterDateTimeLiteral(s)
	}
}

func (s *DateTimeLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitDateTimeLiteral(s)
	}
}

func (s *DateTimeLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitDateTimeLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type StringLiteralContext struct {
	LiteralContext
}

func NewStringLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *StringLiteralContext {
	var p = new(StringLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *StringLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *StringLiteralContext) STRING() antlr.TerminalNode {
	return s.GetToken(FhirPathParserSTRING, 0)
}

func (s *StringLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterStringLiteral(s)
	}
}

func (s *StringLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitStringLiteral(s)
	}
}

func (s *StringLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitStringLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type DateLiteralContext struct {
	LiteralContext
}

func NewDateLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *DateLiteralContext {
	var p = new(DateLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *DateLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *DateLiteralContext) DATE() antlr.TerminalNode {
	return s.GetToken(FhirPathParserDATE, 0)
}

func (s *DateLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterDateLiteral(s)
	}
}

func (s *DateLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitDateLiteral(s)
	}
}

func (s *DateLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitDateLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type BooleanLiteralContext struct {
	LiteralContext
}

func NewBooleanLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *BooleanLiteralContext {
	var p = new(BooleanLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *BooleanLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *BooleanLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterBooleanLiteral(s)
	}
}

func (s *BooleanLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitBooleanLiteral(s)
	}
}

func (s *BooleanLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitBooleanLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type NumberLiteralContext struct {
	LiteralContext
}

func NewNumberLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *NumberLiteralContext {
	var p = new(NumberLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *NumberLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *NumberLiteralContext) NUMBER() antlr.TerminalNode {
	return s.GetToken(FhirPathParserNUMBER, 0)
}

func (s *NumberLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterNumberLiteral(s)
	}
}

func (s *NumberLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitNumberLiteral(s)
	}
}

func (s *NumberLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitNumberLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

type QuantityLiteralContext struct {
	LiteralContext
}

func NewQuantityLiteralContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *QuantityLiteralContext {
	var p = new(QuantityLiteralContext)

	InitEmptyLiteralContext(&p.LiteralContext)
	p.parser = parser
	p.CopyAll(ctx.(*LiteralContext))

	return p
}

func (s *QuantityLiteralContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *QuantityLiteralContext) Quantity() IQuantityContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IQuantityContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IQuantityContext)
}

func (s *QuantityLiteralContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterQuantityLiteral(s)
	}
}

func (s *QuantityLiteralContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitQuantityLiteral(s)
	}
}

func (s *QuantityLiteralContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitQuantityLiteral(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Literal() (localctx ILiteralContext) {
	localctx = NewLiteralContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 4, FhirPathParserRULE_literal)
	var _la int

	p.SetState(95)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}

	switch p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 4, p.GetParserRuleContext()) {
	case 1:
		localctx = NewNullLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 1)
		{
			p.SetState(86)
			p.Match(FhirPathParserT__29)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}
		{
			p.SetState(87)
			p.Match(FhirPathParserT__30)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 2:
		localctx = NewBooleanLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 2)
		{
			p.SetState(88)
			_la = p.GetTokenStream().LA(1)

			if !(_la == FhirPathParserT__31 || _la == FhirPathParserT__32) {
				p.GetErrorHandler().RecoverInline(p)
			} else {
				p.GetErrorHandler().ReportMatch(p)
				p.Consume()
			}
		}

	case 3:
		localctx = NewStringLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 3)
		{
			p.SetState(89)
			p.Match(FhirPathParserSTRING)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 4:
		localctx = NewNumberLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 4)
		{
			p.SetState(90)
			p.Match(FhirPathParserNUMBER)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 5:
		localctx = NewDateLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 5)
		{
			p.SetState(91)
			p.Match(FhirPathParserDATE)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 6:
		localctx = NewDateTimeLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 6)
		{
			p.SetState(92)
			p.Match(FhirPathParserDATETIME)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 7:
		localctx = NewTimeLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 7)
		{
			p.SetState(93)
			p.Match(FhirPathParserTIME)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 8:
		localctx = NewQuantityLiteralContext(p, localctx)
		p.EnterOuterAlt(localctx, 8)
		{
			p.SetState(94)
			p.Quantity()
		}

	case antlr.ATNInvalidAltNumber:
		goto errorExit
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IExternalConstantContext is an interface to support dynamic dispatch.
type IExternalConstantContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	Identifier() IIdentifierContext
	STRING() antlr.TerminalNode

	// IsExternalConstantContext differentiates from other interfaces.
	IsExternalConstantContext()
}

type ExternalConstantContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyExternalConstantContext() *ExternalConstantContext {
	var p = new(ExternalConstantContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_externalConstant
	return p
}

func InitEmptyExternalConstantContext(p *ExternalConstantContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_externalConstant
}

func (*ExternalConstantContext) IsExternalConstantContext() {}

func NewExternalConstantContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *ExternalConstantContext {
	var p = new(ExternalConstantContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_externalConstant

	return p
}

func (s *ExternalConstantContext) GetParser() antlr.Parser { return s.parser }

func (s *ExternalConstantContext) Identifier() IIdentifierContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IIdentifierContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IIdentifierContext)
}

func (s *ExternalConstantContext) STRING() antlr.TerminalNode {
	return s.GetToken(FhirPathParserSTRING, 0)
}

func (s *ExternalConstantContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ExternalConstantContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *ExternalConstantContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterExternalConstant(s)
	}
}

func (s *ExternalConstantContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitExternalConstant(s)
	}
}

func (s *ExternalConstantContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitExternalConstant(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) ExternalConstant() (localctx IExternalConstantContext) {
	localctx = NewExternalConstantContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 6, FhirPathParserRULE_externalConstant)
	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(97)
		p.Match(FhirPathParserT__33)
		if p.HasError() {
			// Recognition error - abort rule
			goto errorExit
		}
	}
	p.SetState(100)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}

	switch p.GetTokenStream().LA(1) {
	case FhirPathParserT__10, FhirPathParserT__11, FhirPathParserT__21, FhirPathParserT__22, FhirPathParserIDENTIFIER, FhirPathParserDELIMITEDIDENTIFIER:
		{
			p.SetState(98)
			p.Identifier()
		}

	case FhirPathParserSTRING:
		{
			p.SetState(99)
			p.Match(FhirPathParserSTRING)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	default:
		p.SetError(antlr.NewNoViableAltException(p, nil, nil, nil, nil, nil))
		goto errorExit
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IInvocationContext is an interface to support dynamic dispatch.
type IInvocationContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser
	// IsInvocationContext differentiates from other interfaces.
	IsInvocationContext()
}

type InvocationContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyInvocationContext() *InvocationContext {
	var p = new(InvocationContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_invocation
	return p
}

func InitEmptyInvocationContext(p *InvocationContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_invocation
}

func (*InvocationContext) IsInvocationContext() {}

func NewInvocationContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *InvocationContext {
	var p = new(InvocationContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_invocation

	return p
}

func (s *InvocationContext) GetParser() antlr.Parser { return s.parser }

func (s *InvocationContext) CopyAll(ctx *InvocationContext) {
	s.CopyFrom(&ctx.BaseParserRuleContext)
}

func (s *InvocationContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *InvocationContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

type TotalInvocationContext struct {
	InvocationContext
}

func NewTotalInvocationContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *TotalInvocationContext {
	var p = new(TotalInvocationContext)

	InitEmptyInvocationContext(&p.InvocationContext)
	p.parser = parser
	p.CopyAll(ctx.(*InvocationContext))

	return p
}

func (s *TotalInvocationContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *TotalInvocationContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterTotalInvocation(s)
	}
}

func (s *TotalInvocationContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitTotalInvocation(s)
	}
}

func (s *TotalInvocationContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitTotalInvocation(s)

	default:
		return t.VisitChildren(s)
	}
}

type ThisInvocationContext struct {
	InvocationContext
}

func NewThisInvocationContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *ThisInvocationContext {
	var p = new(ThisInvocationContext)

	InitEmptyInvocationContext(&p.InvocationContext)
	p.parser = parser
	p.CopyAll(ctx.(*InvocationContext))

	return p
}

func (s *ThisInvocationContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ThisInvocationContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterThisInvocation(s)
	}
}

func (s *ThisInvocationContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitThisInvocation(s)
	}
}

func (s *ThisInvocationContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitThisInvocation(s)

	default:
		return t.VisitChildren(s)
	}
}

type IndexInvocationContext struct {
	InvocationContext
}

func NewIndexInvocationContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *IndexInvocationContext {
	var p = new(IndexInvocationContext)

	InitEmptyInvocationContext(&p.InvocationContext)
	p.parser = parser
	p.CopyAll(ctx.(*InvocationContext))

	return p
}

func (s *IndexInvocationContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *IndexInvocationContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterIndexInvocation(s)
	}
}

func (s *IndexInvocationContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitIndexInvocation(s)
	}
}

func (s *IndexInvocationContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitIndexInvocation(s)

	default:
		return t.VisitChildren(s)
	}
}

type FunctionInvocationContext struct {
	InvocationContext
}

func NewFunctionInvocationContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *FunctionInvocationContext {
	var p = new(FunctionInvocationContext)

	InitEmptyInvocationContext(&p.InvocationContext)
	p.parser = parser
	p.CopyAll(ctx.(*InvocationContext))

	return p
}

func (s *FunctionInvocationContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *FunctionInvocationContext) Function() IFunctionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IFunctionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IFunctionContext)
}

func (s *FunctionInvocationContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterFunctionInvocation(s)
	}
}

func (s *FunctionInvocationContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitFunctionInvocation(s)
	}
}

func (s *FunctionInvocationContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitFunctionInvocation(s)

	default:
		return t.VisitChildren(s)
	}
}

type MemberInvocationContext struct {
	InvocationContext
}

func NewMemberInvocationContext(parser antlr.Parser, ctx antlr.ParserRuleContext) *MemberInvocationContext {
	var p = new(MemberInvocationContext)

	InitEmptyInvocationContext(&p.InvocationContext)
	p.parser = parser
	p.CopyAll(ctx.(*InvocationContext))

	return p
}

func (s *MemberInvocationContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *MemberInvocationContext) Identifier() IIdentifierContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IIdentifierContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IIdentifierContext)
}

func (s *MemberInvocationContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterMemberInvocation(s)
	}
}

func (s *MemberInvocationContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitMemberInvocation(s)
	}
}

func (s *MemberInvocationContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitMemberInvocation(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Invocation() (localctx IInvocationContext) {
	localctx = NewInvocationContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 8, FhirPathParserRULE_invocation)
	p.SetState(107)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}

	switch p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 6, p.GetParserRuleContext()) {
	case 1:
		localctx = NewMemberInvocationContext(p, localctx)
		p.EnterOuterAlt(localctx, 1)
		{
			p.SetState(102)
			p.Identifier()
		}

	case 2:
		localctx = NewFunctionInvocationContext(p, localctx)
		p.EnterOuterAlt(localctx, 2)
		{
			p.SetState(103)
			p.Function()
		}

	case 3:
		localctx = NewThisInvocationContext(p, localctx)
		p.EnterOuterAlt(localctx, 3)
		{
			p.SetState(104)
			p.Match(FhirPathParserT__34)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 4:
		localctx = NewIndexInvocationContext(p, localctx)
		p.EnterOuterAlt(localctx, 4)
		{
			p.SetState(105)
			p.Match(FhirPathParserT__35)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case 5:
		localctx = NewTotalInvocationContext(p, localctx)
		p.EnterOuterAlt(localctx, 5)
		{
			p.SetState(106)
			p.Match(FhirPathParserT__36)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	case antlr.ATNInvalidAltNumber:
		goto errorExit
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IFunctionContext is an interface to support dynamic dispatch.
type IFunctionContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	Identifier() IIdentifierContext
	ParamList() IParamListContext

	// IsFunctionContext differentiates from other interfaces.
	IsFunctionContext()
}

type FunctionContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyFunctionContext() *FunctionContext {
	var p = new(FunctionContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_function
	return p
}

func InitEmptyFunctionContext(p *FunctionContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_function
}

func (*FunctionContext) IsFunctionContext() {}

func NewFunctionContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *FunctionContext {
	var p = new(FunctionContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_function

	return p
}

func (s *FunctionContext) GetParser() antlr.Parser { return s.parser }

func (s *FunctionContext) Identifier() IIdentifierContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IIdentifierContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IIdentifierContext)
}

func (s *FunctionContext) ParamList() IParamListContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IParamListContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IParamListContext)
}

func (s *FunctionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *FunctionContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *FunctionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterFunction(s)
	}
}

func (s *FunctionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitFunction(s)
	}
}

func (s *FunctionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitFunction(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Function() (localctx IFunctionContext) {
	localctx = NewFunctionContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 10, FhirPathParserRULE_function)
	var _la int

	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(109)
		p.Identifier()
	}
	{
		p.SetState(110)
		p.Match(FhirPathParserT__27)
		if p.HasError() {
			// Recognition error - abort rule
			goto errorExit
		}
	}
	p.SetState(112)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}
	_la = p.GetTokenStream().LA(1)

	if (int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&4575657493346129968) != 0 {
		{
			p.SetState(111)
			p.ParamList()
		}

	}
	{
		p.SetState(114)
		p.Match(FhirPathParserT__28)
		if p.HasError() {
			// Recognition error - abort rule
			goto errorExit
		}
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IParamListContext is an interface to support dynamic dispatch.
type IParamListContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	AllExpression() []IExpressionContext
	Expression(i int) IExpressionContext

	// IsParamListContext differentiates from other interfaces.
	IsParamListContext()
}

type ParamListContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyParamListContext() *ParamListContext {
	var p = new(ParamListContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_paramList
	return p
}

func InitEmptyParamListContext(p *ParamListContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_paramList
}

func (*ParamListContext) IsParamListContext() {}

func NewParamListContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *ParamListContext {
	var p = new(ParamListContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_paramList

	return p
}

func (s *ParamListContext) GetParser() antlr.Parser { return s.parser }

func (s *ParamListContext) AllExpression() []IExpressionContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IExpressionContext); ok {
			len++
		}
	}

	tst := make([]IExpressionContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IExpressionContext); ok {
			tst[i] = t.(IExpressionContext)
			i++
		}
	}

	return tst
}

func (s *ParamListContext) Expression(i int) IExpressionContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IExpressionContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IExpressionContext)
}

func (s *ParamListContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *ParamListContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *ParamListContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterParamList(s)
	}
}

func (s *ParamListContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitParamList(s)
	}
}

func (s *ParamListContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitParamList(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) ParamList() (localctx IParamListContext) {
	localctx = NewParamListContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 12, FhirPathParserRULE_paramList)
	var _la int

	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(116)
		p.expression(0)
	}
	p.SetState(121)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}
	_la = p.GetTokenStream().LA(1)

	for _la == FhirPathParserT__37 {
		{
			p.SetState(117)
			p.Match(FhirPathParserT__37)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}
		{
			p.SetState(118)
			p.expression(0)
		}

		p.SetState(123)
		p.GetErrorHandler().Sync(p)
		if p.HasError() {
			goto errorExit
		}
		_la = p.GetTokenStream().LA(1)
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IQuantityContext is an interface to support dynamic dispatch.
type IQuantityContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	NUMBER() antlr.TerminalNode
	Unit() IUnitContext

	// IsQuantityContext differentiates from other interfaces.
	IsQuantityContext()
}

type QuantityContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyQuantityContext() *QuantityContext {
	var p = new(QuantityContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_quantity
	return p
}

func InitEmptyQuantityContext(p *QuantityContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_quantity
}

func (*QuantityContext) IsQuantityContext() {}

func NewQuantityContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *QuantityContext {
	var p = new(QuantityContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_quantity

	return p
}

func (s *QuantityContext) GetParser() antlr.Parser { return s.parser }

func (s *QuantityContext) NUMBER() antlr.TerminalNode {
	return s.GetToken(FhirPathParserNUMBER, 0)
}

func (s *QuantityContext) Unit() IUnitContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IUnitContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IUnitContext)
}

func (s *QuantityContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *QuantityContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *QuantityContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterQuantity(s)
	}
}

func (s *QuantityContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitQuantity(s)
	}
}

func (s *QuantityContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitQuantity(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Quantity() (localctx IQuantityContext) {
	localctx = NewQuantityContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 14, FhirPathParserRULE_quantity)
	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(124)
		p.Match(FhirPathParserNUMBER)
		if p.HasError() {
			// Recognition error - abort rule
			goto errorExit
		}
	}
	p.SetState(126)
	p.GetErrorHandler().Sync(p)

	if p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 9, p.GetParserRuleContext()) == 1 {
		{
			p.SetState(125)
			p.Unit()
		}

	} else if p.HasError() { // JIM
		goto errorExit
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IUnitContext is an interface to support dynamic dispatch.
type IUnitContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	DateTimePrecision() IDateTimePrecisionContext
	PluralDateTimePrecision() IPluralDateTimePrecisionContext
	STRING() antlr.TerminalNode

	// IsUnitContext differentiates from other interfaces.
	IsUnitContext()
}

type UnitContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyUnitContext() *UnitContext {
	var p = new(UnitContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_unit
	return p
}

func InitEmptyUnitContext(p *UnitContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_unit
}

func (*UnitContext) IsUnitContext() {}

func NewUnitContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *UnitContext {
	var p = new(UnitContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_unit

	return p
}

func (s *UnitContext) GetParser() antlr.Parser { return s.parser }

func (s *UnitContext) DateTimePrecision() IDateTimePrecisionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IDateTimePrecisionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IDateTimePrecisionContext)
}

func (s *UnitContext) PluralDateTimePrecision() IPluralDateTimePrecisionContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IPluralDateTimePrecisionContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IPluralDateTimePrecisionContext)
}

func (s *UnitContext) STRING() antlr.TerminalNode {
	return s.GetToken(FhirPathParserSTRING, 0)
}

func (s *UnitContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *UnitContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *UnitContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterUnit(s)
	}
}

func (s *UnitContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitUnit(s)
	}
}

func (s *UnitContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitUnit(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Unit() (localctx IUnitContext) {
	localctx = NewUnitContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 16, FhirPathParserRULE_unit)
	p.SetState(131)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}

	switch p.GetTokenStream().LA(1) {
	case FhirPathParserT__38, FhirPathParserT__39, FhirPathParserT__40, FhirPathParserT__41, FhirPathParserT__42, FhirPathParserT__43, FhirPathParserT__44, FhirPathParserT__45:
		p.EnterOuterAlt(localctx, 1)
		{
			p.SetState(128)
			p.DateTimePrecision()
		}

	case FhirPathParserT__46, FhirPathParserT__47, FhirPathParserT__48, FhirPathParserT__49, FhirPathParserT__50, FhirPathParserT__51, FhirPathParserT__52, FhirPathParserT__53:
		p.EnterOuterAlt(localctx, 2)
		{
			p.SetState(129)
			p.PluralDateTimePrecision()
		}

	case FhirPathParserSTRING:
		p.EnterOuterAlt(localctx, 3)
		{
			p.SetState(130)
			p.Match(FhirPathParserSTRING)
			if p.HasError() {
				// Recognition error - abort rule
				goto errorExit
			}
		}

	default:
		p.SetError(antlr.NewNoViableAltException(p, nil, nil, nil, nil, nil))
		goto errorExit
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IDateTimePrecisionContext is an interface to support dynamic dispatch.
type IDateTimePrecisionContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser
	// IsDateTimePrecisionContext differentiates from other interfaces.
	IsDateTimePrecisionContext()
}

type DateTimePrecisionContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyDateTimePrecisionContext() *DateTimePrecisionContext {
	var p = new(DateTimePrecisionContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_dateTimePrecision
	return p
}

func InitEmptyDateTimePrecisionContext(p *DateTimePrecisionContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_dateTimePrecision
}

func (*DateTimePrecisionContext) IsDateTimePrecisionContext() {}

func NewDateTimePrecisionContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *DateTimePrecisionContext {
	var p = new(DateTimePrecisionContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_dateTimePrecision

	return p
}

func (s *DateTimePrecisionContext) GetParser() antlr.Parser { return s.parser }
func (s *DateTimePrecisionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *DateTimePrecisionContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *DateTimePrecisionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterDateTimePrecision(s)
	}
}

func (s *DateTimePrecisionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitDateTimePrecision(s)
	}
}

func (s *DateTimePrecisionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitDateTimePrecision(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) DateTimePrecision() (localctx IDateTimePrecisionContext) {
	localctx = NewDateTimePrecisionContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 18, FhirPathParserRULE_dateTimePrecision)
	var _la int

	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(133)
		_la = p.GetTokenStream().LA(1)

		if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&140187732541440) != 0) {
			p.GetErrorHandler().RecoverInline(p)
		} else {
			p.GetErrorHandler().ReportMatch(p)
			p.Consume()
		}
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IPluralDateTimePrecisionContext is an interface to support dynamic dispatch.
type IPluralDateTimePrecisionContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser
	// IsPluralDateTimePrecisionContext differentiates from other interfaces.
	IsPluralDateTimePrecisionContext()
}

type PluralDateTimePrecisionContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyPluralDateTimePrecisionContext() *PluralDateTimePrecisionContext {
	var p = new(PluralDateTimePrecisionContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_pluralDateTimePrecision
	return p
}

func InitEmptyPluralDateTimePrecisionContext(p *PluralDateTimePrecisionContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_pluralDateTimePrecision
}

func (*PluralDateTimePrecisionContext) IsPluralDateTimePrecisionContext() {}

func NewPluralDateTimePrecisionContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *PluralDateTimePrecisionContext {
	var p = new(PluralDateTimePrecisionContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_pluralDateTimePrecision

	return p
}

func (s *PluralDateTimePrecisionContext) GetParser() antlr.Parser { return s.parser }
func (s *PluralDateTimePrecisionContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *PluralDateTimePrecisionContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *PluralDateTimePrecisionContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterPluralDateTimePrecision(s)
	}
}

func (s *PluralDateTimePrecisionContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitPluralDateTimePrecision(s)
	}
}

func (s *PluralDateTimePrecisionContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitPluralDateTimePrecision(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) PluralDateTimePrecision() (localctx IPluralDateTimePrecisionContext) {
	localctx = NewPluralDateTimePrecisionContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 20, FhirPathParserRULE_pluralDateTimePrecision)
	var _la int

	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(135)
		_la = p.GetTokenStream().LA(1)

		if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&35888059530608640) != 0) {
			p.GetErrorHandler().RecoverInline(p)
		} else {
			p.GetErrorHandler().ReportMatch(p)
			p.Consume()
		}
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// ITypeSpecifierContext is an interface to support dynamic dispatch.
type ITypeSpecifierContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	QualifiedIdentifier() IQualifiedIdentifierContext

	// IsTypeSpecifierContext differentiates from other interfaces.
	IsTypeSpecifierContext()
}

type TypeSpecifierContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyTypeSpecifierContext() *TypeSpecifierContext {
	var p = new(TypeSpecifierContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_typeSpecifier
	return p
}

func InitEmptyTypeSpecifierContext(p *TypeSpecifierContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_typeSpecifier
}

func (*TypeSpecifierContext) IsTypeSpecifierContext() {}

func NewTypeSpecifierContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *TypeSpecifierContext {
	var p = new(TypeSpecifierContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_typeSpecifier

	return p
}

func (s *TypeSpecifierContext) GetParser() antlr.Parser { return s.parser }

func (s *TypeSpecifierContext) QualifiedIdentifier() IQualifiedIdentifierContext {
	var t antlr.RuleContext
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IQualifiedIdentifierContext); ok {
			t = ctx.(antlr.RuleContext)
			break
		}
	}

	if t == nil {
		return nil
	}

	return t.(IQualifiedIdentifierContext)
}

func (s *TypeSpecifierContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *TypeSpecifierContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *TypeSpecifierContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterTypeSpecifier(s)
	}
}

func (s *TypeSpecifierContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitTypeSpecifier(s)
	}
}

func (s *TypeSpecifierContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitTypeSpecifier(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) TypeSpecifier() (localctx ITypeSpecifierContext) {
	localctx = NewTypeSpecifierContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 22, FhirPathParserRULE_typeSpecifier)
	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(137)
		p.QualifiedIdentifier()
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IQualifiedIdentifierContext is an interface to support dynamic dispatch.
type IQualifiedIdentifierContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	AllIdentifier() []IIdentifierContext
	Identifier(i int) IIdentifierContext

	// IsQualifiedIdentifierContext differentiates from other interfaces.
	IsQualifiedIdentifierContext()
}

type QualifiedIdentifierContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyQualifiedIdentifierContext() *QualifiedIdentifierContext {
	var p = new(QualifiedIdentifierContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_qualifiedIdentifier
	return p
}

func InitEmptyQualifiedIdentifierContext(p *QualifiedIdentifierContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_qualifiedIdentifier
}

func (*QualifiedIdentifierContext) IsQualifiedIdentifierContext() {}

func NewQualifiedIdentifierContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *QualifiedIdentifierContext {
	var p = new(QualifiedIdentifierContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_qualifiedIdentifier

	return p
}

func (s *QualifiedIdentifierContext) GetParser() antlr.Parser { return s.parser }

func (s *QualifiedIdentifierContext) AllIdentifier() []IIdentifierContext {
	children := s.GetChildren()
	len := 0
	for _, ctx := range children {
		if _, ok := ctx.(IIdentifierContext); ok {
			len++
		}
	}

	tst := make([]IIdentifierContext, len)
	i := 0
	for _, ctx := range children {
		if t, ok := ctx.(IIdentifierContext); ok {
			tst[i] = t.(IIdentifierContext)
			i++
		}
	}

	return tst
}

func (s *QualifiedIdentifierContext) Identifier(i int) IIdentifierContext {
	var t antlr.RuleContext
	j := 0
	for _, ctx := range s.GetChildren() {
		if _, ok := ctx.(IIdentifierContext); ok {
			if j == i {
				t = ctx.(antlr.RuleContext)
				break
			}
			j++
		}
	}

	if t == nil {
		return nil
	}

	return t.(IIdentifierContext)
}

func (s *QualifiedIdentifierContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *QualifiedIdentifierContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *QualifiedIdentifierContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterQualifiedIdentifier(s)
	}
}

func (s *QualifiedIdentifierContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitQualifiedIdentifier(s)
	}
}

func (s *QualifiedIdentifierContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitQualifiedIdentifier(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) QualifiedIdentifier() (localctx IQualifiedIdentifierContext) {
	localctx = NewQualifiedIdentifierContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 24, FhirPathParserRULE_qualifiedIdentifier)
	var _alt int

	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(139)
		p.Identifier()
	}
	p.SetState(144)
	p.GetErrorHandler().Sync(p)
	if p.HasError() {
		goto errorExit
	}
	_alt = p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 11, p.GetParserRuleContext())
	if p.HasError() {
		goto errorExit
	}
	for _alt != 2 && _alt != antlr.ATNInvalidAltNumber {
		if _alt == 1 {
			{
				p.SetState(140)
				p.Match(FhirPathParserT__0)
				if p.HasError() {
					// Recognition error - abort rule
					goto errorExit
				}
			}
			{
				p.SetState(141)
				p.Identifier()
			}

		}
		p.SetState(146)
		p.GetErrorHandler().Sync(p)
		if p.HasError() {
			goto errorExit
		}
		_alt = p.GetInterpreter().AdaptivePredict(p.BaseParser, p.GetTokenStream(), 11, p.GetParserRuleContext())
		if p.HasError() {
			goto errorExit
		}
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

// IIdentifierContext is an interface to support dynamic dispatch.
type IIdentifierContext interface {
	antlr.ParserRuleContext

	// GetParser returns the parser.
	GetParser() antlr.Parser

	// Getter signatures
	IDENTIFIER() antlr.TerminalNode
	DELIMITEDIDENTIFIER() antlr.TerminalNode

	// IsIdentifierContext differentiates from other interfaces.
	IsIdentifierContext()
}

type IdentifierContext struct {
	antlr.BaseParserRuleContext
	parser antlr.Parser
}

func NewEmptyIdentifierContext() *IdentifierContext {
	var p = new(IdentifierContext)
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_identifier
	return p
}

func InitEmptyIdentifierContext(p *IdentifierContext) {
	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, nil, -1)
	p.RuleIndex = FhirPathParserRULE_identifier
}

func (*IdentifierContext) IsIdentifierContext() {}

func NewIdentifierContext(parser antlr.Parser, parent antlr.ParserRuleContext, invokingState int) *IdentifierContext {
	var p = new(IdentifierContext)

	antlr.InitBaseParserRuleContext(&p.BaseParserRuleContext, parent, invokingState)

	p.parser = parser
	p.RuleIndex = FhirPathParserRULE_identifier

	return p
}

func (s *IdentifierContext) GetParser() antlr.Parser { return s.parser }

func (s *IdentifierContext) IDENTIFIER() antlr.TerminalNode {
	return s.GetToken(FhirPathParserIDENTIFIER, 0)
}

func (s *IdentifierContext) DELIMITEDIDENTIFIER() antlr.TerminalNode {
	return s.GetToken(FhirPathParserDELIMITEDIDENTIFIER, 0)
}

func (s *IdentifierContext) GetRuleContext() antlr.RuleContext {
	return s
}

func (s *IdentifierContext) ToStringTree(ruleNames []string, recog antlr.Recognizer) string {
	return antlr.TreesStringTree(s, ruleNames, recog)
}

func (s *IdentifierContext) EnterRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.EnterIdentifier(s)
	}
}

func (s *IdentifierContext) ExitRule(listener antlr.ParseTreeListener) {
	if listenerT, ok := listener.(FhirPathListener); ok {
		listenerT.ExitIdentifier(s)
	}
}

func (s *IdentifierContext) Accept(visitor antlr.ParseTreeVisitor) interface{} {
	switch t := visitor.(type) {
	case FhirPathVisitor:
		return t.VisitIdentifier(s)

	default:
		return t.VisitChildren(s)
	}
}

func (p *FhirPathParser) Identifier() (localctx IIdentifierContext) {
	localctx = NewIdentifierContext(p, p.GetParserRuleContext(), p.GetState())
	p.EnterRule(localctx, 26, FhirPathParserRULE_identifier)
	var _la int

	p.EnterOuterAlt(localctx, 1)
	{
		p.SetState(147)
		_la = p.GetTokenStream().LA(1)

		if !((int64(_la) & ^0x3f) == 0 && ((int64(1)<<_la)&864691128467724288) != 0) {
			p.GetErrorHandler().RecoverInline(p)
		} else {
			p.GetErrorHandler().ReportMatch(p)
			p.Consume()
		}
	}

errorExit:
	if p.HasError() {
		v := p.GetError()
		localctx.SetException(v)
		p.GetErrorHandler().ReportError(p, v)
		p.GetErrorHandler().Recover(p, v)
		p.SetError(nil)
	}
	p.ExitRule()
	return localctx
	goto errorExit // Trick to prevent compiler error if the label is not used
}

func (p *FhirPathParser) Sempred(localctx antlr.RuleContext, ruleIndex, predIndex int) bool {
	switch ruleIndex {
	case 0:
		var t *ExpressionContext = nil
		if localctx != nil {
			t = localctx.(*ExpressionContext)
		}
		return p.Expression_Sempred(t, predIndex)

	default:
		panic("No predicate with index: " + fmt.Sprint(ruleIndex))
	}
}

func (p *FhirPathParser) Expression_Sempred(localctx antlr.RuleContext, predIndex int) bool {
	switch predIndex {
	case 0:
		return p.Precpred(p.GetParserRuleContext(), 10)

	case 1:
		return p.Precpred(p.GetParserRuleContext(), 9)

	case 2:
		return p.Precpred(p.GetParserRuleContext(), 7)

	case 3:
		return p.Precpred(p.GetParserRuleContext(), 6)

	case 4:
		return p.Precpred(p.GetParserRuleContext(), 5)

	case 5:
		return p.Precpred(p.GetParserRuleContext(), 4)

	case 6:
		return p.Precpred(p.GetParserRuleContext(), 3)

	case 7:
		return p.Precpred(p.GetParserRuleContext(), 2)

	case 8:
		return p.Precpred(p.GetParserRuleContext(), 1)

	case 9:
		return p.Precpred(p.GetParserRuleContext(), 13)

	case 10:
		return p.Precpred(p.GetParserRuleContext(), 12)

	case 11:
		return p.Precpred(p.GetParserRuleContext(), 8)

	default:
		panic("No predicate with index: " + fmt.Sprint(predIndex))
	}
}
