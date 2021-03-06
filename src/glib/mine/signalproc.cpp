/**
 * Copyright (c) 2015, Jozef Stefan Institute, Quintelligence d.o.o. and contributors
 * All rights reserved.
 * 
 * This source code is licensed under the FreeBSD license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace TSignalProc {

/////////////////////////////////////////////////
// Simple Online Moving Variance
void TVarSimple::Load(TSIn& SIn) {
    *this = TVarSimple(SIn);
}

void TVarSimple::Save(TSOut& SOut) const {
    OldM.Save(SOut);
    NewM.Save(SOut);
    OldS.Save(SOut);
    NewS.Save(SOut);
    N.Save(SOut);
}

void TVarSimple::Update(const double& InVal) {   
    // See Knuth TAOCP vol 2, 3rd edition, page 232
    N++;
    if (N == 1) {
        OldM = NewM = InVal;
        OldS = 0.0;
    } else {
        NewM = OldM + (InVal - OldM) / N;
        NewS = OldS + (InVal - OldM)*(InVal - NewM);
        // set up for next iteration
        OldM = NewM;
        OldS = NewS;
    }
}
   
/////////////////////////////////////////////////
// Online Moving Average
void TMa::AddVal(const double& InVal) {
    // first count in the new value
    Count++;
    // update the moving average
    const double Delta = InVal - Ma;
    Ma = Ma + Delta / Count;
}

void TMa::DeleteVal(const double& OutVal) {
    // if we delete we must have something
    EAssert(Count > 0);
    // update the count
    Count--;
    // update the moving average
    if (Count == 0) {
        // in case no more values, reset
        Ma = 0;
    } else {
        // else readjust
        const double Delta = OutVal - Ma;
        Ma = Ma - Delta / Count;
    }
}

void TMa::Load(TSIn& SIn) {
    *this = TMa(SIn);
}

void TMa::Save(TSOut& SOut) const {
    Count.Save(SOut);
    Ma.Save(SOut);
    TmMSecs.Save(SOut);
}

void TMa::Update(const double& InVal, const uint64& InTmMSecs, 
        const TFltV& OutValV, const TUInt64V& OutTmMSecsV){

    // delete old values
    for (const double OutVal : OutValV) { DeleteVal(OutVal); }
    // add the new value
    AddVal(InVal);
    // update time stamp
    TmMSecs = InTmMSecs;
}

void TMa::Update(const TFltV& InValV, const TUInt64V& InTmMSecsV,
        const TFltV& OutValV, const TUInt64V& OutTmMSecs) {
    
    // delete old values
    for (const double OutVal : OutValV) { DeleteVal(OutVal); }
    // add the new value
    for (const double InVal : InValV) { AddVal(InVal); }
    // update time stamp with the largest of the new ones
    // (we cannot assume any order in the input values)
    for (const uint64 InTmMSecs : InTmMSecsV) {
        if (InTmMSecs > TmMSecs) { TmMSecs = InTmMSecs; }
    }    
}

/////////////////////////////////////////////////
// Online Sum 
void TSum::Load(TSIn& SIn) {
    *this = TSum(SIn);
}

void TSum::Save(TSOut& SOut) const {
    Sum.Save(SOut); 
    TmMSecs.Save(SOut);
}

void TSum::Update(const double& InVal, const uint64& InTmMSecs, const TFltV& OutValV, const TUInt64V& OutTmMSecsV){
    // remove old values from the sum
    for (const double OutVal : OutValV) { Sum -= OutVal; }
    // add the new value to the resulting sum    
    Sum += InVal;
    // update time stamp
    TmMSecs = InTmMSecs;
}

void TSum::Update(const TFltV& InValV, const TUInt64V& InTmMSecsV, const TFltV& OutValV, const TUInt64V& OutTmMSecs) {
    // remove old values from the sum
    for (const double OutVal : OutValV) { Sum -= OutVal; }
    // add new values to the sum
    for (const double InVal : InValV) { Sum += InVal; }
    // update time stamp with the largest of the new ones
    // (we cannot assume any order in the input values)
    for (const uint64 InTmMSecs : InTmMSecsV) {
        if (InTmMSecs > TmMSecs) { TmMSecs = InTmMSecs; }
    }
}

/////////////////////////////////////////////////
// Online Sum of sparse vectors
void TSumSpVec::AddVal(const TIntFltKdV& SpV) {
    TIntFltKdV NewSum;
    TLinAlg::LinComb(1, Sum, 1, SpV, NewSum);
    Sum = NewSum;    
}

void TSumSpVec::DelVal(const TIntFltKdV& SpV) {
    TIntFltKdV NewSum;
    TLinAlg::LinComb(1, Sum, -1, SpV, NewSum);
    Sum = NewSum;        
}

void TSumSpVec::Load(TSIn& SIn) {
    *this = TSumSpVec(SIn);
}

void TSumSpVec::Save(TSOut& SOut) const {
    Sum.Save(SOut);
    TmMSecs.Save(SOut);
}

void TSumSpVec::Update(const TVec<TIntFltKd>& InVal, const uint64& InTmMSecs,
        const TVec<TIntFltKdV>& OutValV, const TUInt64V& OutTmMSecsV) {

    // remove old values from the sum
    for (const TIntFltKdV& OutSpV: OutValV) {
        DelVal(OutSpV);
    }
    // add new values to the sum
    AddVal(InVal);
    // update timestamp
    TmMSecs = InTmMSecs;
}

void TSumSpVec::Update(const TVec<TIntFltKdV>& InValV, const TUInt64V& InTmMSecsV,
        const TVec<TIntFltKdV>& OutValV, const TUInt64V& OutTmMSecs) {

    // remove old values from the sum
    for (const TIntFltKdV& OutSpV: OutValV) {
        DelVal(OutSpV);
    }
    // add new values to the sum
    for (const TIntFltKdV& InSpV: InValV) {
        AddVal(InSpV);
    }
    // update timestamp
    TmMSecs = InTmMSecsV.Last();
}

PJsonVal TSumSpVec::GetJson() const {
    PJsonVal arr = TJsonVal::NewArr();
    for (int i = 0; i < Sum.Len(); i++) {
        PJsonVal tmp = TJsonVal::NewObj();
        tmp->AddToObj("Idx", Sum[i].Key);
        tmp->AddToObj("Val", Sum[i].Dat);
        arr->AddToArr(tmp);
    }
    PJsonVal res = TJsonVal::NewObj();
    res->AddToObj("Sum", arr);
    res->AddToObj("Tm", TmMSecs);
    return res;
}

/////////////////////////////////////////////////
// Online Min
void TMin::AddVal(const double& InVal, const uint64& InTmMSecs) {
    // First we remove all old min candidates that are bigger then the latest value
    while (!AllValV.Empty() && AllValV.Last().Val1 >= InVal) {
        AllValV.DelLast();
    }
    // Then we remember the new minimum candidate
    AllValV.Add(TFltUInt64Pr(InVal, InTmMSecs));    
}

void TMin::DelVal(const uint64& OutTmMSecs) {
    // forget all candidates older then the outgoing timestamp
    while (AllValV[0].Val2 <= OutTmMSecs) {
        AllValV.Del(0);
    }
}

void TMin::Load(TSIn& SIn) {
    *this = TMin(SIn);
}

void TMin::Save(TSOut& SOut) const {
    Min.Save(SOut);
    TmMSecs.Save(SOut);
    AllValV.Save(SOut);
}

void TMin::Update(const double& InVal, const uint64& InTmMSecs, const TFltV& OutValV, const TUInt64V& OutTmMSecsV) {
    /// Add new candidates
    AddVal(InVal, InTmMSecs);
    /// Forget old candidates
    if (!OutTmMSecsV.Empty()) { DelVal(OutTmMSecsV.Last()); }
    /// smallest candidate is the current min
    Min = AllValV[0].Val1;
    /// remember the current timestamp
    TmMSecs = InTmMSecs;
}

void TMin::Update(const TFltV& InValV, const TUInt64V& InTmMSecsV, const TFltV& OutValV, const TUInt64V& OutTmMSecsV) {
    /// Add new candidates
    for (int InValN = 0; InValN < InValV.Len(); InValN++) {
        AddVal(InValV[InValN], InTmMSecsV[InValN]);
    }
    /// Forget old candidates
    if (!OutTmMSecsV.Empty()) { DelVal(OutTmMSecsV.Last()); }
    /// smallest candidate is the current min
    Min = AllValV[0].Val1;
    /// remember the current timestamp if we have any new ones
    if (!InTmMSecsV.Empty()) { TmMSecs = InTmMSecsV.Last(); }
}

/////////////////////////////////////////////////
// Online Max 
void TMax::AddVal(const double& InVal, const uint64& InTmMSecs) {
    // First we remove all old max candidates that are bigger then the latest value
    while (!AllValV.Empty() && AllValV[AllValV.Len() - 1].Val1 <= InVal) {
        AllValV.DelLast();
    }
    // Then we remember the new maximum candidate
    AllValV.Add(TFltUInt64Pr(InVal, InTmMSecs));
}

void TMax::DelVal(const uint64& OutTmMSecs) {
    // forget all candidates older then the outgoing timestamp
    while (AllValV[0].Val2 <= OutTmMSecs) {
        // pop front
        AllValV.Del(0);
    }    
}

void TMax::Load(TSIn& SIn) {
    *this = TMax(SIn);
}

void TMax::Save(TSOut& SOut) const {
    // parameters
    Max.Save(SOut);
    TmMSecs.Save(SOut);
    AllValV.Save(SOut);
}
void TMax::Update(const double& InVal, const uint64& InTmMSecs, const TFltV& OutValV, const TUInt64V& OutTmMSecsV){
    /// Add new candidates
    AddVal(InVal, InTmMSecs);
    /// Forget old candidates
    if (!OutTmMSecsV.Empty()) { DelVal(OutTmMSecsV.Last()); }
    /// largest candidate is the current max
    Max = AllValV[0].Val1;
    /// remember the current timestamp
    TmMSecs = InTmMSecs;
}

void TMax::Update(const TFltV& InValV, const TUInt64V& InTmMSecsV, const TFltV& OutValV, const TUInt64V& OutTmMSecsV) {
    /// Add new candidates
    for (int InValN = 0; InValN < InValV.Len(); InValN++) {
        AddVal(InValV[InValN], InTmMSecsV[InValN]);
    }
    /// Forget old candidates
    if (!OutTmMSecsV.Empty()) { DelVal(OutTmMSecsV.Last()); }
    /// largest candidate is the current max
    Max = AllValV[0].Val1;
    /// remember the current timestamp if we have any new ones
    if (!InTmMSecsV.Empty()) { TmMSecs = InTmMSecsV.Last(); }
}

/////////////////////////////////////////////////
// Exponential Moving Average
double TEma::GetNi(const double& Alpha, const double& Mi) {
    switch (Type) {
    case etPreviousPoint: return 1.0;
    case etLinear: return (1 - Mi) / Alpha;
    case etNextPoint: return Mi;
    }
    throw TExcept::New("Unknown EMA interpolation type");
}

//TODO: compute InitMinMSecs initialization time window from decay factor
TEma::TEma(const double& _Decay, const TEmaType& _Type, const uint64& _InitMinMSecs, 
    const double& _TmInterval): Decay(_Decay), Type(_Type), LastVal(TFlt::Mn),
    TmInterval(_TmInterval), InitP(false), InitMinMSecs(_InitMinMSecs) { }

//TODO: compute InitMinMSecs initialization time window from decay factor
TEma::TEma(const TEmaType& _Type, const uint64& _InitMinMSecs,const double& _TmInterval):
    Type(_Type), LastVal(TFlt::Mn), TmInterval(_TmInterval), InitP(false), 
    InitMinMSecs(_InitMinMSecs) { }

TEma::TEma(const PJsonVal& ParamVal) : LastVal(TFlt::Mn), InitP(false) {
    // type
    TStr TypeStr = ParamVal->GetObjStr("emaType");
    if (TypeStr == "previous") { 
        Type = etPreviousPoint;
    } else if (TypeStr == "linear") {
        Type = etLinear;
    } else if (TypeStr == "next") {
        Type = etNextPoint;
    } else {
        throw TExcept::New("Unknown ema type " + TypeStr);
    }
    // rest
    TmInterval = ParamVal->GetObjNum("interval");
    InitMinMSecs = ParamVal->GetObjInt("initWindow", 0);
}

TEma::TEma(TSIn& SIn) : Decay(SIn), LastVal(SIn), Ema(SIn), TmMSecs(SIn), InitP(SIn),
InitMinMSecs(SIn), InitValV(SIn), InitMSecsV(SIn) {

    TInt TypeI; TypeI.Load(SIn);
    Type = static_cast<TEmaType>((int)TypeI);
    TFlt TmIntervalFlt; TmIntervalFlt.Load(SIn); TmInterval = TmIntervalFlt;
}

void TEma::Load(TSIn& SIn) {
    *this = TEma(SIn);
}

void TEma::Save(TSOut& SOut) const {
    // parameters
    Decay.Save(SOut);
    LastVal.Save(SOut);
    Ema.Save(SOut);
    TmMSecs.Save(SOut);
    InitP.Save(SOut);
    InitMinMSecs.Save(SOut);
    InitValV.Save(SOut);
    InitMSecsV.Save(SOut);
    // TODO: Use macro for saving enum (SaveEnum, LoadEnum)
    // TODO: change TmInterval from double to TFlt
    // PROBLEM: After changing TmInterval from double to TFlt Qminer crashes hard!
    TInt TypeI = Type; // TEmaType 
    TypeI.Save(SOut);
    TFlt TmIntervalFlt = TmInterval; // double
    TmIntervalFlt.Save(SOut);;
}

void TEma::Update(const double& Val, const uint64& NewTmMSecs) {
    double TmInterval1;
    // EMA(first_point) = first_point (no smoothing is possible)
    if (InitMinMSecs == 0) {
        if (LastVal == TFlt::Mn) { LastVal = Val; Ema = Val; TmMSecs = NewTmMSecs; InitP = true;  return; }
    }
    if(NewTmMSecs == TmMSecs) {
        TmInterval1 = 1.0;
    } else{
        TmInterval1 = (double)(NewTmMSecs - TmMSecs);
    }
    if (InitP) {
        // computer parameters for EMA
        double Alpha;
        if (Decay == 0.0) {         
            Alpha = TmInterval1 / TmInterval;
        } else {            
            Alpha = TmInterval1 / TmInterval  * (-1.0) * TMath::Log(Decay);
        }
        const double Mi = exp(-Alpha);
        const double Ni = GetNi(Alpha, Mi);
        // compute new ema
        Ema = Mi*Ema + (Ni - Mi)*LastVal + (1.0 - Ni)*Val;
    } else {
        // update buffers
        InitValV.Add(Val);
        InitMSecsV.Add(NewTmMSecs);
        // initialize when enough data
        const uint64 StartInitMSecs = InitMSecsV[0] + InitMinMSecs;
        if (StartInitMSecs < NewTmMSecs) {
            // Initialize using "buildup time interval",
            //TODO: check how interpolation type influences this code
            const int Vals = InitMSecsV.Len();  
            // compute weights for each value in buffer
            TFltV WeightV(Vals, 0);
            for (int ValN = 0; ValN < Vals; ValN++) {
                const double Alpha = (double)(TmInterval1) / Decay;
                WeightV.Add(exp(-Alpha));
            }
            // normalize weights so they sum to 1.0
            TLinAlg::NormalizeL1(WeightV);
            // compute initial value of EMA as weighted sum
            Ema = TLinAlg::DotProduct(WeightV, InitValV);
            // mark that we are done and clean up after us
            InitP = true; InitValV.Clr(); InitMSecsV.Clr();
        }
    }
    // update last value
    LastVal = Val;
    // update curret time
    TmMSecs = NewTmMSecs;
}

void TEma::Reset() {
    InitP = false;
    LastVal = TFlt::Mn;
    Ema = 0.0;
    TmMSecs = 0;
    InitValV.Gen(0);
    InitMSecsV.Gen(0);
}

/////////////////////////////////////////////////
// Exponential Moving Average - for sparse vectors

double TEmaSpVec::GetNi(const double& Alpha, const double& Mi) {
    switch (Type) {
    case etPreviousPoint: return 1.0;
    case etLinear: return (1 - Mi) / Alpha;
    case etNextPoint: return Mi;
    }
    throw TExcept::New("Unknown EMA interpolation type");
}

TEmaSpVec::TEmaSpVec(const PJsonVal& ParamVal) : LastVal(), InitP(false) {
    // type
    TStr TypeStr = ParamVal->GetObjStr("emaType");
    if (TypeStr == "previous") {
        Type = etPreviousPoint;
    } else if (TypeStr == "linear") {
        Type = etLinear;
    } else if (TypeStr == "next") {
        Type = etNextPoint;
    } else {
        throw TExcept::New("Unknown ema type " + TypeStr);
    }
    // rest
    TmInterval = ParamVal->GetObjNum("interval");
    Cutoff = ParamVal->GetObjNum("cutoff", 0.0001);
    InitMinMSecs = ParamVal->GetObjInt("initWindow", 0);
}

TEmaSpVec::TEmaSpVec(TSIn& SIn) : LastVal(SIn), Ema(SIn), TmMSecs(SIn), 
    TmInterval(SIn), Cutoff(SIn), InitP(SIn),
    InitMinMSecs(SIn), InitValV(SIn), InitMSecsV(SIn) {

    TInt TypeI; TypeI.Load(SIn);
    Type = static_cast<TEmaType>((int)TypeI);
    //TFlt TmIntervalFlt; TmIntervalFlt.Load(SIn); TmInterval = TmIntervalFlt;
    //TFlt CutoffFlt; CutoffFlt.Load(SIn); Cutoff = CutoffFlt;
}

void TEmaSpVec::Load(TSIn& SIn) {
    *this = TEmaSpVec(SIn);
}

void TEmaSpVec::Save(TSOut& SOut) const {
    // parameters
    LastVal.Save(SOut);
    Ema.Save(SOut);
    TmMSecs.Save(SOut);
    TmInterval.Save(SOut);
    Cutoff.Save(SOut);
    InitP.Save(SOut);
    InitMinMSecs.Save(SOut);
    InitValV.Save(SOut);
    InitMSecsV.Save(SOut);
    // TODO: Use macro for saving enum (SaveEnum, LoadEnum)
    // TODO: change TmInterval from double to TFlt
    // PROBLEM: After changing TmInterval from double to TFlt Qminer crashes hard!
    TInt TypeI = Type; // TEmaType 
    TypeI.Save(SOut);
    //TFlt TmIntervalFlt = TmInterval; // double
    //TmIntervalFlt.Save(SOut);
    //TFlt CutoffFlt = Cutoff; // double
    //CutoffFlt.Save(SOut);
}

void TEmaSpVec::Update(const TIntFltKdV& Val, const uint64& NewTmMSecs) {
    double TmInterval1;
    // EMA(first_point) = first_point (no smoothing is possible)
    if (InitMinMSecs == 0) {
        if (LastVal.Empty()) { 
            LastVal = Val; 
            Ema = Val; 
            TmMSecs = NewTmMSecs; 
            InitP = true;  
            return; 
        }
    }
    if (NewTmMSecs == TmMSecs) {
        TmInterval1 = 1.0;
    } else {
        TmInterval1 = (double)(NewTmMSecs - TmMSecs);
    }
    if (InitP) {
        // compute parameters for EMA
        double Alpha = TmInterval1 / TmInterval;
        const double Mi = exp(-Alpha);
        const double Ni = GetNi(Alpha, Mi);
        // compute new ema
        //Ema = Mi*Ema + (Ni - Mi)*LastVal + (1.0 - Ni)*Val;
        TIntFltKdV Tmp;
        TLinAlg::LinComb(Mi, Ema, Ni - Mi, LastVal, Tmp);       
        TLinAlg::LinComb(1, Tmp, 1.0 - Ni, Val, Ema);
    } else {
        // update buffers
        InitValV.Add(Val);
        InitMSecsV.Add(NewTmMSecs);
        // initialize when enough data
        const uint64 StartInitMSecs = InitMSecsV[0] + InitMinMSecs;
        if (StartInitMSecs < NewTmMSecs) {
            // Initialize using "buildup time interval",
            //TODO: check how interpolation type influences this code
            const int Vals = InitMSecsV.Len();
            // compute weights for each value in buffer
            TFltV WeightV(Vals, 0);
            for (int ValN = 0; ValN < Vals; ValN++) {
                const double Alpha = (double)(TmInterval1);
                WeightV.Add(exp(-Alpha));
            }
            // normalize weights so they sum to 1.0
            TLinAlg::NormalizeL1(WeightV);
            // compute initial value of EMA as weighted sum
            //Ema = TLinAlg::DotProduct(WeightV, InitValV);
            TIntFltKdV Tmp;
            for (int i = 0; i < WeightV.Len(); i++) {
                TIntFltKdV Tmp2;
                TLinAlg::LinComb(1, Tmp, WeightV[i], InitValV[i], Tmp2);
                Tmp = Tmp2;
            }
            Ema = Tmp;

            // mark that we are done and clean up after us
            InitP = true; 
            InitValV.Clr();
            InitMSecsV.Clr();
        }
    }
    // remove dimensions bellow cutoff
    TIntFltKdV TmpEma;
    //printf("cutoff %f\n", Cutoff.Val);
    for (int i = 0; i < Ema.Len(); i++) {
        if (TFlt::Abs(Ema[i].Dat.Val) >= Cutoff) {
            TmpEma.Add(Ema[i]);
        }
    }
    Ema = TmpEma;

    // update last value
    LastVal = Val;
    // update current time
    TmMSecs = NewTmMSecs;
}

void TEmaSpVec::Reset() {
    InitP = false;
    LastVal.Clr();
    Ema.Clr();
    TmMSecs = 0;
    InitValV.Gen(0);
    InitMSecsV.Gen(0);
}

PJsonVal TEmaSpVec::GetJson() const {
    PJsonVal arr = TJsonVal::NewArr();
    for (int i = 0; i < Ema.Len(); i++) {
        PJsonVal tmp = TJsonVal::NewObj();
        tmp->AddToObj("Idx", Ema[i].Key);
        tmp->AddToObj("Val", Ema[i].Dat);
        arr->AddToArr(tmp);
    }
    PJsonVal res = TJsonVal::NewObj();
    res->AddToObj("Sum", arr);
    res->AddToObj("Tm", TmMSecs);
    return res;
}

/////////////////////////////////////////////////
// Online Moving Standard M2
void TVar::AddVal(const double& InVal) {
    // increase count
    Count++;
    // update variance parameters
    const double Delta = InVal - Ma;
    Ma = Ma + Delta / (double)Count;
    M2 = M2 + Delta * (InVal - Ma);    
}

void TVar::DelVal(const double& OutVal) {
    EAssert(Count > 0);
    // decrease count of element we are computing variance from
    Count--;
    if (Count == 0) {
        // no more elements, just reset
        Reset();
    } else {
        // decrease parameters
        const double Delta = OutVal - Ma;
        Ma = Ma - Delta / (double)Count;
        M2 = M2 - Delta * (OutVal - Ma);
    }
}

void TVar::Load(TSIn& SIn) {
    *this = TVar(SIn);
}

void TVar::Save(TSOut& SOut) const {
    Count.Save(SOut);
    Ma.Save(SOut); 
    M2.Save(SOut);
    VarVal.Save(SOut);
    TmMSecs.Save(SOut); 
}

void TVar::Update(const double& InVal, const uint64& InTmMSecs, const TFltV& OutValV, const TUInt64V& OutTmMSecsV) {
    // remove old values from the parameters
    for (int ValN = 0; ValN < OutValV.Len(); ValN++) { DelVal(OutValV[ValN]); }
    // add new value to the parameters
    AddVal(InVal);
    // update variance
    UpdateVar();
    // update current timestamp
    TmMSecs = InTmMSecs;
}

void TVar::Update(const TFltV& InValV, const TUInt64V& InTmMSecsV, const TFltV& OutValV, const TUInt64V& OutTmMSecsV) {
    // remove old values from the parameters
    for (int ValN = 0; ValN < OutValV.Len(); ValN++) { DelVal(OutValV[ValN]); }
    // add new value to the parameters
    for (int ValN = 0; ValN < InValV.Len(); ValN++) { AddVal(InValV[ValN]); }
    // update variance
    UpdateVar();    
    // update current timestamp
    if (!InTmMSecsV.Empty()) { TmMSecs = InTmMSecsV.Last(); }
}

/////////////////////////////////////////////////
// Online Moving Covariance 
void TCov::AddVal(const double& InValX, const double& InValY) {
    // increase count
    Count++;
    // update parameters
    MaX = MaX + InValX;
    MaY = MaY + InValY;
    const double Delta = (InValX - MaX / (double)Count) * (InValY - MaY / (double)Count);
    M2 = M2 + Delta;
}

void TCov::DelVal(const double& OutValX, const double& OutValY) {
    EAssert(Count > 0);
    // decrease count of elements we are computing covariance from
    Count--;
    if (Count == 0) {
        // no more elements, just reset
        Reset();        
    } else {
        // update parameters
        MaX = MaX - OutValX;
        MaY = MaY - OutValY;
        // here we use (count + 1) since we did not remove value yet
        const double Delta = (OutValX - MaX / ((double)Count + 1.0)) * (OutValY - MaY / ((double)Count + 1.0));
        M2 = M2 - Delta;
    }
}

void TCov::Load(TSIn& SIn) {
    *this = TCov(SIn);
}

void TCov::Save(TSOut& SOut) const {
    Count.Save(SOut);
    MaX.Save(SOut); 
    MaY.Save(SOut); 
    M2.Save(SOut);
    CovVal.Save(SOut);
    TmMSecs.Save(SOut); 
}

void TCov::Update(const double& InValX, const double& InValY, const uint64& InTmMSecs, 
        const TFltV& OutValVX, const TFltV& OutValVY, const TUInt64V& OutTmMSecsV){

    // remove old values from the parameters
    for (int ValN = 0; ValN < OutValVX.Len(); ValN++) {
        DelVal(OutValVX[ValN], OutValVY[ValN]);
    }
    // add new value to the parameters
    AddVal(InValX, InValY);
    // update variance
    UpdateCov();
    // update current timestamp
    TmMSecs = InTmMSecs;
}

void TCov::Update(const TFltV& InValVX, const TFltV& InValVY, const TUInt64V& InTmMSecsV, 
        const TFltV& OutValVX, const TFltV& OutValVY, const TUInt64V& OutTmMSecsV) {

    // remove old values from the parameters
    for (int ValN = 0; ValN < OutValVX.Len(); ValN++) {
        DelVal(OutValVX[ValN], OutValVY[ValN]);
    }
    // add new value to the parameters
    for (int ValN = 0; ValN < InValVX.Len(); ValN++) {
        AddVal(InValVX[ValN], InValVY[ValN]);
    }
    // update variance
    UpdateCov();    
    // update current timestamp
    if (!InTmMSecsV.Empty()) { TmMSecs = InTmMSecsV.Last(); }
}

/////////////////////////////////////////
// Time series interpolator interface
PInterpolator TInterpolator::New(const TStr& InterpolatorType) {
    if (InterpolatorType == TPreviousPoint::GetType()) {
        return TPreviousPoint::New();
    }    
    else if (InterpolatorType == TLinear::GetType()) {
        return TLinear::New();
    }
    else if (InterpolatorType == TCurrentPoint::GetType()) {
        return TCurrentPoint::New();
    }
    throw TExcept::New("Unknown interpolator type " + InterpolatorType);
}

PInterpolator TInterpolator::Load(TSIn& SIn) {
    TStr InterpolatorType(SIn);
    if (InterpolatorType == TPreviousPoint::GetType()) {
        return TPreviousPoint::New(SIn);
    }
    else if (InterpolatorType == TLinear::GetType()) {
        return TLinear::New(SIn);
    }
    else if (InterpolatorType == TCurrentPoint::GetType()) {
        return TCurrentPoint::New(SIn);
    }
    throw TExcept::New("Unknown interpolator type " + InterpolatorType);
}

/////////////////////////////////////////
// Buffered interpolator
TBufferedInterpolator::TBufferedInterpolator(const TStr& _InterpolatorType):
        TInterpolator(_InterpolatorType),
        Buff() {}

TBufferedInterpolator::TBufferedInterpolator(const TStr& _InterpolatorType, TSIn& SIn) :
        TInterpolator(_InterpolatorType),
        Buff(SIn) {}

void TBufferedInterpolator::Save(TSOut& SOut) const {
    TInterpolator::Save(SOut);
    Buff.Save(SOut);
}

void TBufferedInterpolator::AddPoint(const double& Val, const uint64& Tm) {
    EAssertR(!TFlt::IsNan(Val), "TBufferedInterpolator::AddPoint: got NaN value!");

    // check if the new point can be added
    if (!Buff.Empty()) {
        const TUInt64FltPr& LastRec = Buff.GetNewest();
        EAssertR(LastRec.Val1 < Tm || (LastRec.Val1 == Tm && LastRec.Val2 == Val),
            "New point has a timestamp lower then the last point in the buffer, or same with different values " + TTm::GetTmFromDateTimeInt((uint)LastRec.Val1).GetStr() + " >= " + TTm::GetTmFromDateTimeInt((uint)Tm).GetStr() + "!");
    }

    // add the new point
    Buff.Add(TUInt64FltPr(Tm, Val));
}

/////////////////////////////////////////
// Previous point interpolator.
// Interpolate by returning last seen value
TPreviousPoint::TPreviousPoint():
        TBufferedInterpolator(TPreviousPoint::GetType()) {}

TPreviousPoint::TPreviousPoint(TSIn& SIn):
    TBufferedInterpolator(GetType(), SIn) {}

void TPreviousPoint::SetNextInterpTm(const uint64& Time) {
    // TODO optimize
    while (Buff.Len() > 1 && Buff.GetOldest(1).Val1 < Time) {
        Buff.DelOldest();
    }
}

double TPreviousPoint::Interpolate(const uint64& Tm) const {
    IAssertR(CanInterpolate(Tm), "TPreviousPoint::Interpolate: Time not in the desired interval!");
    return Buff.GetOldest().Val2;
}

bool TPreviousPoint::CanInterpolate(const uint64& Tm) const {
    return (!Buff.Empty() && Buff.GetOldest().Val1 == Tm) ||
                (Buff.Len() >= 2 && Buff.GetOldest().Val1 <= Tm && Buff.GetOldest(1).Val1 >= Tm);
}

/////////////////////////////////////////
// Current point interpolator.
TCurrentPoint::TCurrentPoint():
        TBufferedInterpolator(TCurrentPoint::GetType()) {}

TCurrentPoint::TCurrentPoint(TSIn& SIn):
    TBufferedInterpolator(GetType(), SIn) {}

void TCurrentPoint::SetNextInterpTm(const uint64& Tm) {
    // at least one past (or current time) record needs to be in the buffer
    bool Change = false;
    while (Buff.Len() >= 2 && Buff.GetOldest(1).Val1 <= Tm) {
        Buff.DelOldest();
        Change = true;
    }
    if (Change) {
        EAssertR(CanInterpolate(Tm), "WTF!? Current point interpolator cannot intrpolate after setting new time!");
    }
    // when the loop finishes we have at least 1 record in the buffer
    // with a timestamp <= Tm
}

double TCurrentPoint::Interpolate(const uint64& Tm) const {
    IAssertR(CanInterpolate(Tm), "TCurrentPoint::Interpolate: Time not in the desired interval!");
    return Buff.GetOldest().Val2;
}

bool TCurrentPoint::CanInterpolate(const uint64& Tm) const {
    return !Buff.Empty() && Buff.GetOldest().Val1 <= Tm;
}


/////////////////////////////////////////
// Time series linear interpolator
TLinear::TLinear():
        TBufferedInterpolator(TLinear::GetType()) {}

TLinear::TLinear(TSIn& SIn):
    TBufferedInterpolator(GetType(), SIn) {}

void TLinear::SetNextInterpTm(const uint64& Time) {
    while (Buff.Len() > 1 && Buff.GetOldest(1).Val1 <= Time) {
        Buff.DelOldest();
    }
}

double TLinear::Interpolate(const uint64& Tm) const {
    AssertR(CanInterpolate(Tm), "TLinear::Interpolate: Time not in the desired interval!");

    const TUInt64FltPr& PrevRec = Buff.GetOldest();
    if (Tm == PrevRec.Val1) { return PrevRec.Val2; }
    const TUInt64FltPr& NextRec = Buff.GetOldest(1);

    // don't need to check if the times of the previous rec and next rec are equal since if
    // that is true Tm will be equal to PrevRec.Tm and the correct result will be returned
    const double Result = PrevRec.Val2 + ((double) (Tm - PrevRec.Val1) / (NextRec.Val1 - PrevRec.Val1)) * (NextRec.Val2 - PrevRec.Val2);
    EAssertR(!TFlt::IsNan(Result), "TLinear: result of interpolation is NaN!");
    return Result;
}

bool TLinear::CanInterpolate(const uint64& Tm) const {
    return (!Buff.Empty() && Buff.GetOldest().Val1 == Tm) ||
            (Buff.Len() >= 2 && Buff.GetOldest().Val1 <= Tm && Tm <= Buff.GetOldest(1).Val1);
}

///////////////////////////////////////////////////////////////////
// Neural Networks - Neuron
TRnd TNNet::TNeuron::Rnd = 0;

TNNet::TNeuron::TNeuron(){
}

TNNet::TNeuron::TNeuron(TInt OutputsN, TInt MyId, TTFunc TransFunc){

    TFuncNm = TransFunc;
    for(int c = 0; c < OutputsN; ++c){
        // define the edges, 0 element is weight, 1 element is weight delta
        TFlt RandWeight = RandomWeight();
        OutEdgeV.Add(TIntFltFltTr(c, RandWeight, 0.0));
        SumDeltaWeight.Add(0.0);
    }

    Id = MyId;
}
TNNet::TNeuron::TNeuron(TSIn& SIn){
    OutputVal.Load(SIn);
    Gradient.Load(SIn);
    TFuncNm = LoadEnum<TTFunc>(SIn);
    SumDeltaWeight.Load(SIn);
    OutEdgeV.Load(SIn);
    Id.Load(SIn);
}

void TNNet::TNeuron::FeedFwd(const TLayer& PrevLayer){
    TFlt SumIn = 0.0;
    // get the previous layer's outputs and sum them up
    for(int NeuronN = 0; NeuronN < PrevLayer.GetNeuronN(); ++NeuronN){
        SumIn += PrevLayer.GetOutVal(NeuronN) * 
                PrevLayer.GetWeight(NeuronN, Id);
    }
    // not sure if static. maybe different fcn for output nodes
    OutputVal = TNeuron::TransferFcn(SumIn);
}

TFlt TNNet::TNeuron::TransferFcn(TFlt Sum){
    switch (TFuncNm){
        case tanHyper:
            // tanh output range [-1.0..1.0]
            // training data should be scaled to what the transfer function can handle
           return tanh(Sum);
        case softPlus:
            // the softplus function
            return log(1.0 + exp(Sum));
        case sigmoid:
           // sigmoid output range [0.0..1.0]
           // training data should be scaled to what the transfer function can handle
           return 1.0 / (1.0 + exp(-Sum));
        case fastTanh:
           // sigmoid output range [-1.0..1.0]
           // training data should be scaled to what the transfer function can handle
           return Sum / (1.0 + fabs(Sum));
        case fastSigmoid:
           // sigmoid output range [0.0..1.0]
           // training data should be scaled to what the transfer function can handle
           return (Sum / 2.0) / (1.0 + fabs(Sum)) + 0.5;
        case linear:
            return Sum;         
    };
    throw TExcept::New("Unknown transfer function type");
}

TFlt TNNet::TNeuron::TransferFcnDeriv(TFlt Sum){
    switch (TFuncNm){
        case tanHyper:
            // tanh derivative approximation
            return 1.0 - tanh(Sum) * tanh(Sum);
        case softPlus:
            // softplus derivative
            return 1.0 / (1.0 + exp(-Sum));
        case sigmoid:{
           double Fun = 1.0 / (1.0 + exp(-Sum));
           return Fun * (1.0 - Fun);
        }
        case fastTanh:
           return 1.0 / ((1.0 + fabs(Sum)) * (1.0 + fabs(Sum)));
        case fastSigmoid:
           return 1.0 / (2.0 * (1.0 + fabs(Sum)) * (1.0 + fabs(Sum)));
        case linear:
            return 1;         
    };
    throw TExcept::New("Unknown transfer function type");
}

void TNNet::TNeuron::CalcOutGradient(TFlt TargVal){
    TFlt Delta = TargVal - OutputVal;
    // TODO: different ways of calculating gradients
    Gradient = Delta * TNeuron::TransferFcnDeriv(OutputVal);
}

void TNNet::TNeuron::CalcHiddenGradient(const TLayer& NextLayer){
    // calculate error by summing the derivatives of the weights next layer
    TFlt DerivsOfWeights = SumDOW(NextLayer);
    Gradient = DerivsOfWeights * TNeuron::TransferFcnDeriv(OutputVal);
}

TFlt TNNet::TNeuron::SumDOW(const TLayer& NextLayer) const{
    TFlt sum = 0.0;
    // sum our contributions of the errors at the nodes we feed
    for(int NeuronN = 0; NeuronN < NextLayer.GetNeuronN() - 1; ++NeuronN){
        // weight from us to next layer neuron times its gradient
        sum += GetWeight(NeuronN) * NextLayer.GetGradient(NeuronN);
    }

    return sum;
}

void TNNet::TNeuron::UpdateInputWeights(TLayer& PrevLayer, const TFlt& LearnRate, const TFlt& Momentum, const TBool& UpdateWeights){
    // the weights to be updated are in the OutEdgeV
    // in the neurons in the preceding layer
    for(int NeuronN = 0; NeuronN < PrevLayer.GetNeuronN(); ++NeuronN){
        TNeuron& Neuron = PrevLayer.GetNeuron(NeuronN);
        TFlt OldDeltaWeight = Neuron.GetDeltaWeight(Id);
        //TFlt OldWeight = Neuron.GetWeight(Id);
        TFlt OldSumDeltaWeight = Neuron.GetSumDeltaWeight(Id);
        
        TFlt NewDeltaWeight = 
                // individual input magnified by the gradient and train rate
                LearnRate
                * Neuron.GetOutVal()
                * Gradient
                // add momentum = fraction of previous delta weight, if we are not in batch mode
                + (UpdateWeights  && OldSumDeltaWeight == 0.0 ? Momentum : TFlt())
                * OldDeltaWeight;
        
        if(UpdateWeights){
            if(OldSumDeltaWeight != 0.0){
                NewDeltaWeight = OldSumDeltaWeight + NewDeltaWeight;
                Neuron.SetSumDeltaWeight(Id, 0.0);
            }
            Neuron.SetDeltaWeight(Id, NewDeltaWeight);
            Neuron.UpdateWeight(Id, NewDeltaWeight);
       }
        else{
            Neuron.SumDeltaWeights(Id, NewDeltaWeight);
        }
    }
}
void TNNet::TNeuron::Save(TSOut& SOut) {
    OutputVal.Save(SOut);
    Gradient.Save(SOut);
    SaveEnum<TTFunc>(SOut, TFuncNm);
    SumDeltaWeight.Save(SOut);
    OutEdgeV.Save(SOut);
    Id.Save(SOut);
}
    
/////////////////////////////////////////////////////////////////////////
//// Neural Networks - Layer of Neurons
// TODO: why do we need this empty constructor?
TNNet::TLayer::TLayer(){
}

TNNet::TLayer::TLayer(const TInt& NeuronsN, const TInt& OutputsN, const TTFunc& TransFunc){
    // Add neurons to the layer, plus bias neuron
    for(int NeuronN = 0; NeuronN <= NeuronsN; ++NeuronN){
        NeuronV.Add(TNeuron(OutputsN, NeuronN, TransFunc));
        // debugging
        /*printf("Made a neuron!");
        printf(" Neuron N: %d", NeuronN);
        printf(" Neuron H: %d", NeuronV.Len());
        printf("\n");*/
    } 
    // Force the bias node's output value to 1.0
    NeuronV.Last().SetOutVal(1.0);
    //printf("\n");
}
TNNet::TLayer::TLayer(TSIn& SIn){
    NeuronV.Load(SIn);
}
void TNNet::TLayer::Save(TSOut& SOut) {
    NeuronV.Save(SOut);
}

/////////////////////////////////////////////////////////////////////////
//// Neural Networks - Neural Net
TNNet::TNNet(const TIntV& LayoutV, const TFlt& _LearnRate, 
            const TFlt& _Momentum, const TTFunc& TFuncHiddenL,
            const TTFunc& TFuncOutL){
    // get number of layers
    int LayersN = LayoutV.Len();
    LearnRate = _LearnRate;
    Momentum = _Momentum;
    // create each layer
    for(int LayerN = 0; LayerN < LayersN; ++LayerN){
        // Get number of neurons in the next layer
        // set number of output connections, if output layer then no output connections
        TInt OutputsN = LayerN == LayersN - 1 ? TInt(0) : LayoutV[LayerN + 1];
        // set transfer functions for hidden and output layers
        TTFunc TransFunc = LayerN == LayersN - 1 ? TFuncOutL : TFuncHiddenL;
        TInt NeuronsN = LayoutV[LayerN];
        // Add a layer to the net
        LayerV.Add(TLayer(NeuronsN, OutputsN, TransFunc));
        //for debugging
        //printf("LayerV.Len(): %d \n", LayerV.Len() );
    }
}

TNNet::TNNet(TSIn& SIn):
        LearnRate(SIn),
        Momentum(SIn) {
    LayerV.Load(SIn);
}

PNNet TNNet::Load(TSIn& SIn) {
    return new TNNet(SIn);
}

void TNNet::FeedFwd(const TFltV& InValV){
    // check if number of input values same as number of input neurons
    EAssertR(InValV.Len() == LayerV[0].GetNeuronN() - 1, "InValV must be of equal length than the first layer!");
    // assign input values to input neurons
    for(int InputN = 0; InputN < InValV.Len(); ++InputN){
        LayerV[0].SetOutVal(InputN, InValV[InputN]);
    }

    // forward propagation
    for(int LayerN = 1; LayerN < LayerV.Len(); ++LayerN){
        TLayer& PrevLayer = LayerV[LayerN - 1];
        for(int NeuronN = 0; NeuronN < LayerV[LayerN].GetNeuronN() - 1; ++NeuronN){
            LayerV[LayerN].FeedFwd(NeuronN, PrevLayer);
        }
    }
}

void TNNet::BackProp(const TFltV& TargValV, const TBool& UpdateWeights){
    // calculate overall net error (RMS of output neuron errors)
    TLayer& OutputLayer = LayerV.Last();
    Error = 0.0;

    EAssertR(TargValV.Len() == OutputLayer.GetNeuronN() - 1, "TargValV must be of equal length than the last layer!");
    for(int NeuronN = 0; NeuronN < OutputLayer.GetNeuronN() - 1; ++NeuronN){
        TFlt Delta = TargValV[NeuronN] - OutputLayer.GetOutVal(NeuronN);
        Error += Delta * Delta;
    }
    Error /= OutputLayer.GetNeuronN() - 1;
    Error = sqrt(Error); // RMS
    
    // recent avg error measurement
    RecentAvgError = (RecentAvgError * RecentAvgSmoothingFactor + Error)
            / (RecentAvgSmoothingFactor + 1.0);
    // Calculate output layer gradients
    for(int NeuronN = 0; NeuronN < OutputLayer.GetNeuronN() - 1; ++NeuronN){
        OutputLayer.CalcOutGradient(NeuronN, TargValV[NeuronN]);
    }        
    // Calculate gradients on hidden layers
    for(int LayerN = LayerV.Len() - 2; LayerN > 0; --LayerN){
        TLayer& HiddenLayer = LayerV[LayerN];
        TLayer& NextLayer = LayerV[LayerN + 1];

        for(int NeuronN = 0; NeuronN < HiddenLayer.GetNeuronN() - 1; ++NeuronN){
            HiddenLayer.CalcHiddenGradient(NeuronN, NextLayer);
        }

    }
    // For all layers from output to first hidden layer
    // update connection weights
    for(int LayerN = LayerV.Len() - 1; LayerN > 0; --LayerN){
        TLayer& Layer = LayerV[LayerN];
        TLayer& PrevLayer = LayerV[LayerN - 1];

        for(int NeuronN = 0; NeuronN < Layer.GetNeuronN() - 1; ++NeuronN){
            Layer.UpdateInputWeights(NeuronN, PrevLayer, LearnRate, Momentum, UpdateWeights);
        }
    }
}

void TNNet::GetResults(TFltV& ResultV) const{
    ResultV.Clr(true, -1);

    for(int NeuronN = 0; NeuronN < LayerV.Last().GetNeuronN() - 1; ++NeuronN){
        ResultV.Add(LayerV.Last().GetOutVal(NeuronN));
    }
}

void TNNet::Save(TSOut& SOut) const {

    // Save model variables
    LearnRate.Save(SOut);
    Momentum.Save(SOut);
    LayerV.Save(SOut);
}

TStr TNNet::GetFunction(const TTFunc& FuncEnum) {
    TStr FuncString;
    if (FuncEnum == TSignalProc::TTFunc::tanHyper) {
        FuncString = "tanHyper";
    } else if (FuncEnum == TSignalProc::TTFunc::sigmoid) {
        FuncString = "sigmoid";
    } else if (FuncEnum == TSignalProc::TTFunc::fastTanh) {
        FuncString = "fastTanh";
    } else if (FuncEnum == TSignalProc::TTFunc::softPlus) {
        FuncString = "softPlus";
    } else if (FuncEnum == TSignalProc::TTFunc::fastSigmoid) {
        FuncString = "fastSigmoid";
    } else if (FuncEnum == TSignalProc::TTFunc::linear) {
        FuncString = "linear";
    } else {
        throw TExcept::New("Unknown transfer function type " + FuncString);
    }
    return FuncString;
}

///////////////////////////////////////////////////////////////////
// Recursive Linear Regression
TRecLinReg::TRecLinReg(const TRecLinReg& LinReg):
        ForgetFact(LinReg.ForgetFact),
        RegFact(LinReg.RegFact),
        P(LinReg.P),
        Coeffs(LinReg.Coeffs) {}

TRecLinReg::TRecLinReg(const TRecLinReg&& LinReg):
        ForgetFact(std::move(LinReg.ForgetFact)),
        RegFact(std::move(LinReg.RegFact)),
        P(std::move(LinReg.P)),
        Coeffs(std::move(LinReg.Coeffs)) {}

TRecLinReg::TRecLinReg(TSIn& SIn):
        ForgetFact(SIn),
        RegFact(SIn) {
    P.Load(SIn);
    Coeffs.Load(SIn);
}

TRecLinReg::TRecLinReg(const int& Dim, const double& _RegFact,
            const double& _ForgetFact):
        ForgetFact(_ForgetFact),
        RegFact(_RegFact),
        P(TFullMatrix::Identity(Dim) / RegFact),
        Coeffs(Dim, true) {}

void TRecLinReg::Save(TSOut& SOut) const {
    ForgetFact.Save(SOut);
    RegFact.Save(SOut);
    P.Save(SOut);
    Coeffs.Save(SOut);
}

PRecLinReg TRecLinReg::Load(TSIn& SIn) {
    return new TRecLinReg(SIn);
}

TRecLinReg& TRecLinReg::operator =(TRecLinReg LinReg) {
    std::swap(ForgetFact, LinReg.ForgetFact);
    std::swap(P, LinReg.P);
    std::swap(Coeffs, LinReg.Coeffs);

    return *this;
}

double TRecLinReg::Predict(const TFltV& Sample) {
    return Coeffs.DotProduct(Sample);
}

void TRecLinReg::Learn(const TFltV& Sample, const double& SampleVal) {
    double PredVal = Predict(Sample);

    TVector x(Sample);

    TVector Px = P * x;
    double xPx = Px.DotProduct(Sample);

    /*
     * linreg.P = (linreg.P - (Px * Px') / (linreg.lambda + xPx)) / linreg.lambda;
     * linreg.w = linreg.w + Px*((y - y_hat)/(linreg.lambda + xPx));
     */
    P = (P - (Px*Px.GetT()) / (ForgetFact + xPx)) / ForgetFact;
    Coeffs += Px*((SampleVal - PredVal) / (ForgetFact + xPx));
}

void TRecLinReg::GetCoeffs(TFltV& Coef) const {
    Coef = Coeffs.GetVec();
}

bool TRecLinReg::HasNaN() const {
    int Dim = Coeffs.Len();
    for (int ElN = 0; ElN < Dim; ElN++) {
        if (Coeffs[ElN].IsNan()) {
            return true;
        }
    }
    return false;
}

void TOnlineHistogram::Init(const double& LBound, const double& UBound, const int& Bins, const bool& AddNegInf, const bool& AddPosInf) {
    int TotalBins = Bins + (AddNegInf ? 1 : 0) + (AddPosInf ? 1 : 0);
    Counts.Gen(TotalBins); // sets to zero
    Bounds.Gen(TotalBins + 1, 0);
    if (AddNegInf) { Bounds.Add(TFlt::NInf); }
    for (int ElN = 0; ElN <= Bins; ElN++) {
        Bounds.Add(LBound + ElN * (UBound - LBound) / Bins);
    }
    if (AddPosInf) { Bounds.Add(TFlt::PInf); }
}

void TOnlineHistogram::Reset() {
    for (int ElN = 0; ElN < Counts.Len(); ElN++) {
        Counts[ElN] = 0; Count = 0;
    }
}

TOnlineHistogram::TOnlineHistogram(const PJsonVal& ParamVal) {
    EAssertR(ParamVal->IsObjKey("lowerBound"), "TOnlineHistogram: lowerBound key missing!");
    EAssertR(ParamVal->IsObjKey("upperBound"), "TOnlineHistogram: upperBound key missing!");
    // bounded lowest point
    TFlt LBound = ParamVal->GetObjNum("lowerBound");
    // bounded highest point
    TFlt UBound = ParamVal->GetObjNum("upperBound");
    EAssertR(LBound < UBound, "TOnlineHistogram: Lower bound should be smaller than upper bound");
    // number of equal bins ? (not counting possibly infinite ones)
    TInt Bins = ParamVal->GetObjInt("bins", 5);
    EAssertR(Bins > 0, "TOnlineHistogram: Number of bins should be greater than 0");
    // include infinities in the bounds?
    TBool AddNegInf = ParamVal->GetObjBool("addNegInf", false);
    TBool AddPosInf = ParamVal->GetObjBool("addPosInf", false);
    
    MinCount = ParamVal->GetObjInt("initMinCount", 0);

    Init(LBound, UBound, Bins, AddNegInf, AddPosInf);
};


int TOnlineHistogram::FindBin(const double& Val) const {
    int Bins = Bounds.Len() - 1;
    int LBound = 0;
    int UBound = Bins - 1;
    
    // out of bounds
    if ((Val < Bounds[0]) || (Val > Bounds.Last())) { return -1; }
    // the last bound is an exception: the interval is closed from the right
    if (Val == Bounds.Last()) { return UBound; }

    while (LBound <= UBound) {
        int Idx = (LBound + UBound) / 2;
        if ((Val >= Bounds[Idx]) && (Val < Bounds[Idx + 1])) { // value between
            return Idx; 
        } else if (Val < Bounds[Idx]) { // value on the left, move upper bound
            UBound = Idx - 1;
        } else { // Val > Bounds[Idx + 1]
            LBound = Idx + 1;
        }
    }
    return -1;
}

void TOnlineHistogram::Increment(const double& Val) {   
    int Idx = FindBin(Val);
    if (Idx >= 0) { Counts[Idx]++; Count++; }
}

void TOnlineHistogram::Decrement(const double& Val) {
    int Idx = FindBin(Val);
    if (Idx >= 0) { Counts[Idx]--; Count--; }
}

double TOnlineHistogram::GetCount(const double& Val) const {
    int Idx = FindBin(Val);
    return Idx >= 0 ? (double)Counts[Idx] : 0.0;
}

void TOnlineHistogram::Print() const {
    printf("Histogram:\n");
    for (int BinN = 0; BinN < Counts.Len(); BinN++) {
        printf("%g [%g, %g]\n", Counts[BinN].Val, Bounds[BinN].Val, Bounds[BinN + 1].Val);
    }
}

PJsonVal TOnlineHistogram::SaveJson() const {
    PJsonVal Result = TJsonVal::NewObj();
    PJsonVal BoundsArr = TJsonVal::NewArr();
    PJsonVal CountsArr = TJsonVal::NewArr();
    for (int ElN = 0; ElN < Counts.Len(); ElN++) {
        BoundsArr->AddToArr(Bounds[ElN]);
        CountsArr->AddToArr(Counts[ElN]);
    }
    BoundsArr->AddToArr(Bounds.Last());
    Result->AddToObj("bounds", BoundsArr);
    Result->AddToObj("counts", CountsArr);
    return Result;
}

void TTDigest::Update(const double& V) {
    const TFlt Count = 1;
    Update(V, Count);
}

void TTDigest::Update(const double& V, const double& Count) {
    if (TempLast >= TempWeight.Len()) {
        MergeValues();
    }
    TInt N_ = TempLast++;
    TempWeight[N_] = Count;
    TempMean[N_] = V;
    UnmergedSum += Count;
    MergeValues();
}

int TTDigest::GetClusters() const {
    return Mean.Len();
}
double TTDigest::GetQuantile(const double& Q) const {
    double Left = Min;
    double Right = Max;

    if (TotalSum == 0.0) { return -1.0; }
    if (Q <= 0) { return Min; }
    if (Q >= 1) { return Max; }
    if (Last == 0) { return Mean[0]; }

    // calculate boundaries, pick centroid via binary search
    double QSum = Q * TotalSum;

    int N1 = Last + 1;
    int N0 = 0;
    int I = Bisect(MergeMean, QSum, N0, N1);

    if (I > 0) {
        Left = Boundary(I-1, I, Mean, Weight);
    }
    if (I < Last) {
        Right = Boundary(I, I+1, Mean, Weight);
    }
    return Left + (Right - Left) * (QSum - (MergeMean[I-1])) / Weight[I];
};

void TTDigest::MergeValues() {
    if (UnmergedSum == 0.0) {
        return;
    }
    TFltV W = Weight;
    TFltV U = Mean;

    double Sum = 0;

    TempMean.Sort();

    TInt LastN = 0;
    if (TotalSum > 0.0) {
        LastN = Last + 1;
    }

    Last = 0;
    TotalSum += UnmergedSum;
    UnmergedSum = 0.0;

    double NewCentroid = 0;
    int IterI = 0;
    int IterJ = 0;
    // merge existing centroids with added values in temp buffers

    while (IterI < TempLast && IterJ < LastN) {
        if (TempMean[IterI] <= U[IterJ]) {
            Sum += TempWeight[IterI];
            double TW = TempWeight[IterI];
            double TM = TempMean[IterI];
            IterI++;
            NewCentroid = MergeCentroid(Sum, NewCentroid, TW, TM);

        } else {
            Sum += W[IterJ];
            double TW = W[IterJ];
            double TM = U[IterJ];
            IterJ++;
            NewCentroid = MergeCentroid(Sum, NewCentroid, TW, TM);

        }
    }

    while (IterI < TempLast) {
        Sum += TempWeight[IterI];
        double TW = TempWeight[IterI];
        double TM = TempMean[IterI];
        NewCentroid = MergeCentroid(Sum, NewCentroid, TW, TM);
        IterI++;
    }

    // only existing     centroids remain
    while (IterJ < LastN) {
        Sum += W[IterJ];
        double TW = W[IterJ];
        double TM = U[IterJ];
        NewCentroid = MergeCentroid(Sum, NewCentroid, TW, TM);
        IterJ++;
    }

    TempLast = 0;

    // swap pointers for working space and merge space

    Weight = MergeWeight;
    Mean = MergeMean;

    MergeMean = U;
    MergeWeight = W;

    MergeMean[0] = Weight[0];
    MergeWeight[0] = 0;
    for (int Iter = 1; Iter<=Last && Iter<MergeMean.Len(); ++Iter) {
        MergeWeight[Iter] = 0; // zero out merge weights
        MergeMean[Iter] = MergeMean[Iter-1] + Weight[Iter]; // stash cumulative dist
    }

    Min = TMath::Mn(Min, Mean[0]);
    if (LastN < Mean.Len()) {
        Max = TMath::Mx(Max, Mean[LastN]);
    }
}

double TTDigest::MergeCentroid(double& Sum, double& K1, double& Wt, double& Ut) {
    double K2 = Integrate((double)Nc, Sum/TotalSum);
        if (K2 - K1 <= 1.0 || MergeWeight[Last] == 0.0) {
            // merge into existing centroid if centroid index difference (k2-k1)
            // is within 1 or if current centroid is empty
            MergeWeight[Last] += Wt;
            MergeMean[Last] += (Ut - MergeMean[Last]) * Wt / MergeWeight[Last];
        } else {
            // otherwise create a new centroid
            Last = ++Last;
            MergeMean[Last] = Ut;
            MergeWeight[Last] = Wt;
            K1 = Integrate((double)Nc, (Sum - Wt)/TotalSum);
        }

    return K1;
};

double TTDigest::Integrate(const double& Nc, const double& Q_) const {
    // First, scale and bias the quantile domain to [-1, 1]
    // Next, bias and scale the arcsin range to [0, 1]
    // This gives us a [0,1] interpolant following the arcsin shape
    // Finally, multiply by centroid count for centroid scale value
    return Nc * (asin(2 * Q_ - 1) + TMath::Pi / 2) / TMath::Pi;
}

int TTDigest::Bisect(const TFltV& A, const double& X, int& Low, int& Hi) const {
    while (Low < Hi) {
        TInt Mid = (Low + Hi) >> 1;
        if (A[Mid] < X) {
            Low = Mid + 1;
        }
        else {
            Hi = Mid;
        }
    }
    return Low;
}

double TTDigest::Boundary(const int& I, const int& J, const TFltV& U, const TFltV& W) const {
    return U[I] + (U[J] - U[I]) * W[I] / (W[I] + W[J]);
}

int TTDigest::NumTemp(const int& N) const {
    int Lo = 1, Hi = N, Mid;
    while (Lo < Hi) {
        Mid = (Lo + Hi) >> 1;
        if (N > Mid * TMath::Log(Mid) / TMath::Log(2)) {
            Lo = Mid + 1;
        }
        else {
            Hi = Mid;
        }
  }
  return Lo;
}

void TTDigest::Print() const {}

void TTDigest::SaveState(TSOut& SOut) const {
    Nc.Save(SOut);
    Size.Save(SOut);
    Last.Save(SOut);
    TotalSum.Save(SOut);
    Weight.Save(SOut);
    Mean.Save(SOut);
    Min.Save(SOut);
    Max.Save(SOut);
    MergeWeight.Save(SOut);
    MergeMean.Save(SOut);
    Tempsize.Save(SOut);
    UnmergedSum.Save(SOut);
    TempLast.Save(SOut);
    TempWeight.Save(SOut);
    TempMean.Save(SOut);
}

void TTDigest::LoadState(TSIn& SIn) {
    Nc.Load(SIn);
    Size.Load(SIn);
    Last.Load(SIn);
    TotalSum.Load(SIn);
    Weight.Load(SIn);
    Mean.Load(SIn);
    Min.Load(SIn);
    Max.Load(SIn);
    MergeWeight.Load(SIn);
    MergeMean.Load(SIn);
    Tempsize.Load(SIn);
    UnmergedSum.Load(SIn);
    TempLast.Load(SIn);
    TempWeight.Load(SIn);
    TempMean.Load(SIn);
}

void TTDigest::Init(const int& N) {
    Nc = N;
    Max = TFlt::Mn;
    Min = TFlt::Mx;
    UnmergedSum = 0;
    TempLast = 0;

    Size = (int) ceil(Nc * TMath::Pi/2);
    TotalSum = 0;
    Last = 0;

    for (int Iter = 0; Iter < Size; Iter++) {
        Weight.Add(0);
        Mean.Add(0);
        MergeWeight.Add(0);
        MergeMean.Add(0);
    }

    int Tempsize = NumTemp(Nc);
    for (int Iter = 0; Iter < Tempsize; Iter++) {
        TempMean.Add(TFlt::Mx);
        TempWeight.Add(0);
    }
}

TChiSquare::TChiSquare(const PJsonVal& ParamVal): P(TFlt::PInf) {
    // P value is set to infinity by default (null hypothesis is not rejected)
    EAssertR(ParamVal->IsObjKey("degreesOfFreedom"), "TChiSquare: degreesOfFreedom key missing!");
    // degrees of freedom
    DegreesOfFreedom = ParamVal->GetObjInt("degreesOfFreedom");
}

void TChiSquare::Print() const {
    printf("Chi2 = %g", Chi2.Val);
    printf("P = %g", P.Val);    
}

void TChiSquare::Update(const TFltV& OutValVX, const TFltV& OutValVY) {
    TStatFun::ChiSquare(OutValVX, OutValVY, DegreesOfFreedom, Chi2, P);
}

void TChiSquare::LoadState(TSIn& SIn) {
    Chi2.Load(SIn);
    P.Load(SIn);    
}

void TChiSquare::SaveState(TSOut& SOut) const {
    Chi2.Save(SOut);
    P.Save(SOut);
}

///////////////////////////////
/// Slotted histogram
TSlottedHistogram::TSlottedHistogram(const uint64 _Period, const uint64 _Slot, const int _Bins) {
    PeriodLen = _Period;
    SlotGran = _Slot;
    Bins = _Bins;
    uint64 Slots = PeriodLen / SlotGran;
    Dat.Gen((int)Slots);
    for (int i = 0; i < Dat.Len(); i++) {
        Dat[i] = TSignalProc::TOnlineHistogram(0, Bins, Bins, false, false);
    }
}

void TSlottedHistogram::Reset() {
    for (int HistN = 0; HistN < Dat.Len(); HistN++) {
        Dat[HistN].Reset();
    }   
}

void TSlottedHistogram::LoadState(TSIn& SIn) {
    PeriodLen.Load(SIn);
    SlotGran.Load(SIn);
    Bins.Load(SIn);
    Dat.Load(SIn);
}

void TSlottedHistogram::SaveState(TSOut& SOut) const {
    PeriodLen.Save(SOut);
    SlotGran.Save(SOut);
    Bins.Save(SOut);
    Dat.Save(SOut);
}

void TSlottedHistogram::Add(const uint64& Ts, const int& Val) {
    int Idx = GetIdx(Ts);
    Dat[Idx].Increment(Val);
}

void TSlottedHistogram::Remove(const uint64& Ts, const int& Val) {
    int Idx = GetIdx(Ts);
    Dat[Idx].Decrement(Val);
}

void TSlottedHistogram::GetStats(const uint64 TsMin, const uint64 TsMax, TFltV& Dest) {
    EAssertR(TsMax > TsMin, "Invalid period query in TSlottedHistogram. TsMax <= TsMin");
    EAssertR(TsMax - PeriodLen < TsMin, "Invalid period query in TSlottedHistogram. TsMax - period >= TsMin");
    Dest.Clr();
    uint64 TsMin2 = (TsMin / SlotGran) * SlotGran;
    uint64 TsMax2 = (TsMax / SlotGran) * SlotGran;
    for (uint64 i = TsMin2; i <= TsMax2; i += SlotGran) {
        int Idx = GetIdx(i);
        TFltV Tmp;
        Dat[Idx].GetCountV(Tmp);
        if (Dest.Len() < Tmp.Len()) {
            Dest.Gen(Tmp.Len());
        }
        for (int j = 0; j < Tmp.Len(); j++) {
            Dest[j] += Tmp[j];
        }
    }
}

}
