import Models = require("../share/models");
import Utils = require("./utils");
import Statistics = require("./statistics");
import moment = require("moment");

//import Broker = require("./broker");
export class TargetBasePositionManager {
  public sideAPR: string;

  private newWidth: Models.IStdev = null;
  private newQuote: number = null;
  public newShort: number = 0;
  public newMedium: number = 0;
  public newLong: number = 0;
  private fairValue: number = null;
  public latestLong: number = 0;
  private latestMedium: number = 0;
  public latestShort: number = 0;



  public set quoteEwma(quoteEwma: number) {
    this.newQuote = quoteEwma;
  }
  public set widthStdev(widthStdev: Models.IStdev) {
    this.newWidth = widthStdev;
  }

  private _newTargetPosition: number = 0;
  private _lastPosition: number = null;

  private _latest: Models.TargetBasePositionValue = null;
  public get latestTargetPosition(): Models.TargetBasePositionValue {
    return this._latest;
  }

  private  _latestEWMACur: Models.EWMACurrent = null;
  public get latestEMACurrent(): Models.EWMACurrent{
    return this._latestEWMACur;
  }
   constructor(
    private _minTick: number,
    private _dbInsert,
    private _fvEngine,
    private _mgEwmaShort,
    private _mgEwmaMedium,
    private _mgEwmaLong,
    private _mgTBP,
    private _qpRepo,
    private _positionBroker,
    private _uiSnap,
    private _uiSend,
    private _evOn,
    private _evUp,
      initTBP: Models.TargetBasePositionValue[],
      initEWMACur: Models.EWMACurrent[]
    ) {
      if (initTBP.length && typeof initTBP[0].tbp != "undefined") {
        this._latest = initTBP[0];
        console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Loaded from DB:', this._latest.tbp);
      }
      if (initEWMACur.length && typeof initEWMACur[0].currentShort != "undefined") {
        this._latestEWMACur = initEWMACur[0];
        this.newShort = this._latestEWMACur.currentShort;
        this.newLong = this._latestEWMACur.currentLong;
        console.info(new Date().toISOString().slice(11, -1), 'EWMA Short', 'Loaded from DB:', this._latestEWMACur.currentShort);
        console.info(new Date().toISOString().slice(11, -1), 'EWMA Long', 'Loaded from DB:', this._latestEWMACur.currentLong);
      }
    _uiSnap(Models.Topics.TargetBasePosition, () => [this._latest]);
    _uiSnap(Models.Topics.EWMACurrent, () => [this._latestEWMACur]);
    _uiSnap(Models.Topics.EWMAChart, () => [this.fairValue?new Models.EWMAChart(
      this.newWidth,
      this.newQuote?Utils.roundNearest(this.newQuote, this._minTick):null,
      this.newShort?Utils.roundNearest(this.newShort, this._minTick):null,
      this.newMedium?Utils.roundNearest(this.newMedium, this._minTick):null,
      this.newLong?Utils.roundNearest(this.newLong, this._minTick):null,
      this.fairValue?Utils.roundNearest(this.fairValue, this._minTick):null
    ):null]);
    this._evOn('PositionBroker', this.recomputeTargetPosition);
    this._evOn('EWMACurrent', this.updateEwmaValues);
    this._evOn('QuotingParameters', () => setTimeout(() => this.recomputeTargetPosition(), moment.duration(121)));
    setInterval(this.updateEwmaValues, moment.duration(1, 'minutes'));
  }

  private recomputeTargetPosition = () => {
    var params = this._qpRepo();
    const latestReport = this._positionBroker();
    if (params === null || latestReport === null) {
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Unable to compute tbp [ qp | pos ] = [', !!params, '|', !!this._positionBroker.latestReport, ']');
      return;
    }
    const targetBasePosition: number = (params.autoPositionMode === Models.AutoPositionMode.Manual)
      ? (params.percentageValues
        ? params.targetBasePositionPercentage * latestReport.value / 100
        : params.targetBasePosition)
      : ((1 + this._newTargetPosition) / 2) * latestReport.value;

    if (this._latest === null || Math.abs(this._latest.tbp - targetBasePosition) > 1e-4 || this.sideAPR !== this._latest.sideAPR) {
      this._latest = new Models.TargetBasePositionValue(targetBasePosition, this.sideAPR);
      this._evUp('TargetPosition');
      this._uiSend(Models.Topics.TargetBasePosition, this._latest, true);
      this._dbInsert(Models.Topics.TargetBasePosition, this._latest);
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'recalculated', this._latest.tbp);

      if(this._latestEWMACur === null) {
        this._latestEWMACur = new Models.EWMACurrent(this.newShort , this.newLong, this.newMedium);
        this._evUp('EWMACurrent');
        this._uiSend(Models.Topics.EWMACurrent, this._latestEWMACur , true);
        this._dbInsert(Models.Topics.EWMACurrent, this._latestEWMACur );
        console.info(new Date().toISOString().slice(11, -1), 'EWMA Freshened', this._latestEWMACur );
        console.info(new Date().toISOString().slice(11, -1), 'EWMA Freshened', this._latestEWMACur.currentShort , this._latestEWMACur.currentLong );
      }

    //  let fairFV: number = this._fvAgent.latestFairValue.price;
    //this._dbInsert(Models.Topics.EWMACurrent, new Models.EWMACurrent(this.newShort , this.newLong, this.newMedium));
    //  this._uiSend(Models.Topics.FairValue,   fairValue , true);
    //  this._dbInsert(Models.Topics.FairValue,   fairValue );
/*
    if (this._fvAgent.latestFairValue === null) {
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Unable to update ewma');
      return;
    }
*/
    let movement: number = ((this.newShort - this.newLong) / ((this.newShort + this.newLong) / 2)) * 100 ;
//

    //  console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'Find the bug:', mrdebug )
    //  console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'Fair Value recalculated:', this._fvAgent.latestFairValue.price )
//    console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'New Short Value:', (this._ewma.latestShort ) )
    console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'New Long Value:', (this.newLong) )
    console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'recalculated', params.aspvalue )
    console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'Movement', movement )
    console.info(new Date().toISOString().slice(11, -1), 'ASP2', 'Movement', params.aspactive  )


      if(this.newShort > this.newLong) {
        // Going up!
        params.moveit = Models.mMoveit.up;
        console.info(new Date().toISOString().slice(11, -1), 'MoveMent: ',   Models.mMoveit[params.moveit] )
        if (params.upnormallow > movement && params.upnormalhigh < movement) {
          params.movement = Models.mMovemomentum.normal;
        } else if ( movement > params.upmidlow && movement < params.upmidhigh )
        {
          params.movement = Models.mMovemomentum.mid;
        } else if (movement > params.upfastlow )
        {
          params.movement = Models.mMovemomentum.fast;
        }


      } else if(this.newShort  < this.newLong) {
        // Going down
        params.moveit = Models.mMoveit.down;
        console.info(new Date().toISOString().slice(11, -1), 'MoveMent: ',   Models.mMoveit[params.moveit] )
        if (movement > params.dnnormallow  &&  movement < params.dnnormalhigh ) {
          params.movement = Models.mMovemomentum.normal;
        } else if ( movement < params.dnmidlow  &&  movement > params.dnmidhigh )
        {
          params.movement = Models.mMovemomentum.mid;
        } else if (movement < params.dnfastlow )
        {
          params.movement = Models.mMovemomentum.fast;
        }
      } else {
        console.info(new Date().toISOString().slice(11, -1), 'Movement', 'I Suck at math: ' )

      }
      console.info(new Date().toISOString().slice(11, -1), 'Movement', 'Speed: ', Models.mMovemomentum[params.movement] )


    }

  };

  public updateEwmaValues = () => {
    const fv = this._fvEngine();
    if (!fv) {
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Unable to update ewma');
      return;
    }

    //  const params = this._qpRepo();

  //  this.fairValue = this._fvAgent.latestFairValue.price;
  // see above how to get fv
    this.fairValue = fv;






    this.newShort = this._mgEwmaShort(this.fairValue);
    this.newMedium = this._mgEwmaMedium(this.fairValue);
    this.newLong = this._mgEwmaLong(this.fairValue);
    this._newTargetPosition = this._mgTBP(this.fairValue, this.newLong, this.newMedium, this.newShort);
    console.info(new Date().toISOString().slice(11, -1), 'tbp', 'recalculated ewma [ FV | L | M | S ] = [',this.fairValue,'|',this.newLong,'|',this.newMedium,'|',this.newShort,']');
    this.recomputeTargetPosition();

    console.info(new Date().toISOString().slice(11, -1), 'EWMA Freshened', this._latestEWMACur );
    this._dbInsert(Models.Topics.EWMACurrent, new Models.EWMACurrent(this.newShort , this.newLong, this.newMedium));


    this._uiSend(Models.Topics.EWMAChart, new Models.EWMAChart(
      this.newWidth,
      this.newQuote?Utils.roundNearest(this.newQuote, this._minTick):null,
      Utils.roundNearest(this.newShort, this._minTick),
      Utils.roundNearest(this.newMedium, this._minTick),
      Utils.roundNearest(this.newLong, this._minTick),
      Utils.roundNearest(this.fairValue, this._minTick)
    ), true);

    this._dbInsert(Models.Topics.EWMAChart, new Models.RegularFairValue(this.fairValue, this.newLong, this.newMedium, this.newShort));
  };
}
