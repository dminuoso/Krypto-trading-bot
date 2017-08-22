import Models = require("../share/models");
import Utils = require("./utils");
import moment = require("moment");


function computeEwma(newValue: number, previous: number, periods: number): number {
  if (previous !== null) {
      const alpha = 2 / (periods + 1);
      return alpha * newValue + (1 - alpha) * previous;
  }

  return newValue;
}

export class EWMATargetPositionCalculator {
  constructor(
    private _qpRepo,
    initRfv: Models.RegularFairValue[]
  ) {
    if (initRfv !== null && initRfv.length) {
      this.latestLong = initRfv[0].ewmaLong;
      this.latestMedium = initRfv[0].ewmaMedium;
      this.latestShort = initRfv[0].ewmaShort;
    }
  }
  private _SMA3: number[] = [];
  private _SMA33: number[] = [];
  private _SMA33TIME: number[] = [];


  public latestLong: number = null;
  private latestMedium: number = null;
  public latestShort: number = null;

  computeTBP(value: number, newLong: number, newMedium: number, newShort: number): number {
    const params = this._qpRepo();
    this._SMA3.push(value);

    this._SMA3 = this._SMA3.slice(-3);
    this._SMA33 = this._SMA33.slice(-100);
    this._SMA33TIME = this._SMA33TIME.slice(-100);

    console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'SMA Length' , this._SMA33.length );
    //  console.warn(new Date().toISOString().slice(11, -1), 'SMA3', 'SMA1 Value' , this._SMA3[0] );
    //  console.warn(new Date().toISOString().slice(11, -1), 'SMA3', 'SMA P' , (params.safetyP/100) );
    const SMA3 = this._SMA3.reduce((a,b) => a+b) / this._SMA3.length;

    //this._SMA33.push(SMA3);

    var currenttime = new Date();
    var valuetopush = new Array();
    this._SMA33.push(SMA3)
    var currenttime = new Date();
    this._SMA33TIME.push(currenttime.getTime());



/* enter safey modes */
//  var SMAOLD = this._SMA3[3];=
  if (((this._SMA33[this._SMA33.length-1] * 100 / this._SMA33[0]) - 100) > (params.safetyP/100))
  {
    params.mSafeMode = Models.mSafeMode.buy;
    params.safetimestart = currenttime.getTime();
    console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Value BUY' , ((this._SMA33[this._SMA33.length-1] * 100 / this._SMA33[0]) - 100) );
    console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Safety Percent' , (params.safetyP/100) );
  }
  if (((this._SMA33[this._SMA33.length-1] * 100 / this._SMA3[0]) - 100) <  -(params.safetyP/100) )
  {
    params.mSafeMode = Models.mSafeMode.sell;
    params.safetimestart = currenttime.getTime();
    console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Value SELL' , ((this._SMA33[this._SMA33.length-1] * 100 / this._SMA33[0]) -100) );
    console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Safety Percent' , -(params.safetyP/100) );
  }

  console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'SMA3' , SMA3 );
  console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'latest SMA33' , this._SMA33[this._SMA33.length-1]  );
  console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Safety Mode' , Models.mSafeMode[params.mSafeMode] );
/* exit safey modes
    if(
      (SMA3 < this._SMA3[3])

      ) { }
      params.moveit = Models.mMoveit.up;
      console.info(new Date().toISOString().slice(11, -1), 'MoveMent: ',   Models.mMoveit[params.moveit] )
      */
// lets see if its time to get out of safety modes
/* If SMA3(latest) < SMA3(-Z) && timer over
   Quit Safety buy Mode
   */
   var duration  = currenttime.getTime()  - params.safetimestart;
   if ( (this._SMA33[this._SMA33.length-1] < this._SMA33[this._SMA33.length - params.safetytime] )
   &&  (
          duration >= (params.safetimeOver * 60000)

       )
      )
  {

    params.mSafeMode = Models.mSafeMode.unknown;
      console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Safety Mode OVER ' , Models.mSafeMode[params.mSafeMode] );

  } else if ( (this._SMA33[this._SMA33.length-1] > this._SMA33[this._SMA33.length - params.safetytime] )
   &&  (
          duration >= (params.safetimeOver * 60000)

       )
      )
   {
        params.mSafeMode = Models.mSafeMode.unknown;
        console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Safety Mode OVER ' , Models.mSafeMode[params.mSafeMode] );
   }


    let newTargetPosition: number = 0;

        if (params.autoPositionMode === Models.AutoPositionMode.EWMA_LMS) {
          const newTrend = ((SMA3 * 100 / newLong) - 100);
          const newEwmacrossing = ((newShort * 100 / newMedium) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / params.ewmaSensiblityPercentage);
        } else if (params.autoPositionMode === Models.AutoPositionMode.EWMA_LS) {
          newTargetPosition = ((newShort * 100/ newLong) - 100) * (1 / params.ewmaSensiblityPercentage);
        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;

        console.warn(new Date().toISOString().slice(11, -1), 'ASP', 'ASP Value Set to' , params.aspvalue );

        if(params.safetynet &&  params.mSafeMode == Models.mSafeMode.buy ) {

          newTargetPosition = 1;
          console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Activating New Target Position' , newTargetPosition );
        }
        if(params.safetynet &&  params.mSafeMode == Models.mSafeMode.sell ) {
          newTargetPosition = -1;
          console.warn(new Date().toISOString().slice(11, -1), 'SMA33', 'Activating New Target Position' , newTargetPosition );
        }


    return newTargetPosition;
  }

  addNewShortValue(value: number): number {
    this.latestShort = computeEwma(value, this.latestShort, this._qpRepo().shortEwmaPeridos);
    return this.latestShort;
  }

  addNewMediumValue(value: number): number {
    this.latestMedium = computeEwma(value, this.latestMedium, this._qpRepo().mediumEwmaPeridos);
    return this.latestMedium;
  }

  addNewLongValue(value: number): number {
    this.latestLong = computeEwma(value, this.latestLong, this._qpRepo().longEwmaPeridos);
    return this.latestLong;
  }
}

export class EWMAProtectionCalculator {
  constructor(
    private _fvEngine,
    private _qpRepo,
    private _evUp
  ) {
    setInterval(this.onTick, moment.duration(1, "minutes"));
  }

  private onTick = () => {
    var fv = this._fvEngine();
    if (!fv) {
      console.warn(new Date().toISOString().slice(11, -1), 'ewma', 'Unable to compute value');
      return;
    }

    this.setLatest(computeEwma(fv, this._latest, this._qpRepo().quotingEwmaProtectionPeridos));
  };

  private _latest: number = null;
  public get latest() { return this._latest; }
  private setLatest = (v: number) => {
    this._latest = v;
    this._evUp('EWMAProtectionCalculator');
  };
}

export class STDEVProtectionCalculator {
    private _lastBids: number[] = [];
    private _lastAsks: number[] = [];
    private _lastFV: number[] = [];
    private _lastTops: number[] = [];

    private _latest: Models.IStdev = null;
    public get latest() { return this._latest; }

    constructor(
      private _fvEngine,
      private _mgFilter,
      private _qpRepo,
      private _dbInsert,
      private _computeStdevs,
      initMkt: Models.MarketStats[]
    ) {
      if (initMkt !== null) {
        this.initialize(
          initMkt.map((r: Models.MarketStats) => r.fv),
          initMkt.map((r: Models.MarketStats) => r.bid),
          initMkt.map((r: Models.MarketStats) => r.ask)
        );
      }
      setInterval(this.onTick, moment.duration(1, "seconds"));
    }

    private initialize(rfv: number[], mktBids: number[], mktAsks: number[]) {
      const params = this._qpRepo();
      for (let i = 0; i<mktBids.length||i<mktAsks.length;i++)
        if (mktBids[i] && mktAsks[i]) this._lastTops.push(mktBids[i], mktAsks[i]);
      this._lastFV = rfv.slice(-params.quotingStdevProtectionPeriods);
      this._lastTops = this._lastTops.slice(-params.quotingStdevProtectionPeriods * 2);
      this._lastBids = mktBids.slice(-params.quotingStdevProtectionPeriods);
      this._lastAsks = mktAsks.slice(-params.quotingStdevProtectionPeriods);

      this.onSave();
    };

    private onSave = () => {
      if (this._lastFV.length < 2 || this._lastTops.length < 2 || this._lastBids.length < 2 || this._lastAsks.length < 2) return;

      this._latest = <Models.IStdev>this._computeStdevs(
        new Float64Array(this._lastFV),
        new Float64Array(this._lastTops),
        new Float64Array(this._lastBids),
        new Float64Array(this._lastAsks),
        this._qpRepo().quotingStdevProtectionFactor
      );
    };


    private onTick = () => {
        const filteredMkt = this._mgFilter();
        const fv = this._fvEngine();
        if (!fv || filteredMkt == null || !filteredMkt.bids.length || !filteredMkt.asks.length) {
            console.warn(new Date().toISOString().slice(11, -1), 'stdev', 'Unable to compute value');
            return;
        }
        const params = this._qpRepo();
        this._lastFV.push(fv);
        this._lastTops.push(filteredMkt.bids[0].price, filteredMkt.asks[0].price);
        this._lastBids.push(filteredMkt.bids[0].price);
        this._lastAsks.push(filteredMkt.asks[0].price);
        this._lastFV = this._lastFV.slice(-params.quotingStdevProtectionPeriods);
        this._lastTops = this._lastTops.slice(-params.quotingStdevProtectionPeriods * 2);
        this._lastBids = this._lastBids.slice(-params.quotingStdevProtectionPeriods);
        this._lastAsks = this._lastAsks.slice(-params.quotingStdevProtectionPeriods);

        this.onSave();

        this._dbInsert(Models.Topics.MarketData, new Models.MarketStats(fv, filteredMkt.bids[0].price, filteredMkt.asks[0].price, new Date().getTime()), false, undefined, new Date().getTime() - 1000 * params.quotingStdevProtectionPeriods);
    };
}
