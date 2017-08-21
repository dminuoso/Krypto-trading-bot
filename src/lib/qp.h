#ifndef K_QP_H_
#define K_QP_H_

namespace K {
  static json defQP {
    {  "widthPing",                     2                                      },
    {  "widthPingPercentage",           decimal_cast<2>("0.25").getAsDouble()  },
    {  "widthPong",                     2                                      },
    {  "widthPongPercentage",           decimal_cast<2>("0.25").getAsDouble()  },
    {  "widthPercentage",               false                                  },
    {  "bestWidth",                     true                                   },
    {  "buySize",                       decimal_cast<2>("0.02").getAsDouble()  },
    {  "buySizePercentage",             7                                      },
    {  "buySizeMax",                    false                                  },
    {  "sellSize",                      decimal_cast<2>("0.01").getAsDouble()  },
    {  "sellSizePercentage",            7                                      },
    {  "sellSizeMax",                   false                                  },
    {  "pingAt",                        (int)mPingAt::BothSides                },
    {  "pongAt",                        (int)mPongAt::ShortPingFair            },
    {  "mode",                          (int)mQuotingMode::AK47                },
    {  "bullets",                       2                                      },
    {  "range",                         decimal_cast<1>("0.5").getAsDouble()   },
    {  "fvModel",                       (int)mFairValueModel::BBO              },
    {  "targetBasePosition",            1                                      },
    {  "targetBasePositionPercentage",  50                                     },
    {  "positionDivergence",            decimal_cast<1>("0.9").getAsDouble()   },
    {  "positionDivergencePercentage",  21                                     },
    {  "percentageValues",              false                                  },
    {  "autoPositionMode",              (int)mAutoPositionMode::EWMA_LS        },
    {  "aggressivePositionRebalancing", (int)mAPR::Off                         },
    {  "superTrades",                   (int)mSOP::Off                         },
    {  "tradesPerMinute",               decimal_cast<1>("0.9").getAsDouble()   },
    {  "tradeRateSeconds",              69                                     },
    {  "quotingEwmaProtection",         true                                   },
    {  "quotingEwmaProtectionPeridos",  200                                    },
    {  "quotingStdevProtection",        (int)mSTDEV::Off                       },
    {  "quotingStdevBollingerBands",    false                                  },
    {  "quotingStdevProtectionFactor",  decimal_cast<1>("1.0").getAsDouble()   },
    {  "quotingStdevProtectionPeriods", 1200                                   },
    {  "ewmaSensiblityPercentage",      decimal_cast<1>("0.5").getAsDouble()   },
    {  "longEwmaPeridos",               200                                    },
    {  "mediumEwmaPeridos",             100                                    },
    {  "shortEwmaPeridos",              50                                     },
    {  "aprMultiplier",                 2                                      },
    {  "sopWidthMultiplier",            2                                      },
    {  "delayAPI",                      0                                      },
    {  "cancelOrdersAuto",              false                                  },
    {  "cleanPongsAuto",                0                                      },
    {  "profitHourInterval",            decimal_cast<1>("0.5").getAsDouble()   },
    {  "audio",                         false                                  },
    {  "delayUI",                       7                                      },
    {  "moveit",                        (int)mMoveit::unknown                  },
    {  "movement",                      (int)mMovemomentum::unknown            },
    {  "upnormallow",                   decimal_cast<1>("-0.5").getAsDouble()  },
    { "upnormalhigh",                   decimal_cast<1>("0.5").getAsDouble()   },
    {  "upmidlow",                      decimal_cast<1>("0.5").getAsDouble()   },
    {  "upmidhigh",                     decimal_cast<1>("1").getAsDouble()     },
    {  "upfastlow",                     decimal_cast<1>("1").getAsDouble()     },
    {  "dnnormallow",                   decimal_cast<1>("-0.5").getAsDouble()  },
    {  "dnnormalhigh",                  decimal_cast<1>("0.5").getAsDouble()   },
    {  "dnmidlow",                      decimal_cast<1>("-0.5").getAsDouble()  },
    {  "dnmidhigh",                     decimal_cast<1>("-1").getAsDouble()    },
    {  "dnfastlow",                     decimal_cast<1>("-1").getAsDouble()    },
    {  "asp_low",                       decimal_cast<1>("-5").getAsDouble()    },
    {  "asp_high",                      decimal_cast<1>("5").getAsDouble()     },
    {  "aspactive",                     false                                  },
    {  "aspvalue",                      decimal_cast<1>("0").getAsDouble()     },
    {  "safetynet",                     false                                  },
    {  "safeZ",                         decimal_csat<1>("3").getAsDouble()     }

  };
  static vector<string> boolQP = {
    "widthPercentage", "bestWidth", "sellSizeMax", "buySizeMax", "percentageValues",
    "quotingEwmaProtection", "quotingStdevBollingerBands", "cancelOrdersAuto", "audio"
  };
  class QP {
    public:
      static void main(Local<Object> exports) {
        for (json::iterator it = defQP.begin(); it != defQP.end(); ++it) {
          string k = CF::cfString(it.key(), false);
          if (k != "") {
            if (it.value().is_number()) defQP[it.key()] = stod(k);
            else if (it.value().is_boolean()) defQP[it.key()] = (FN::S2u(k) == "TRUE" or k == "1");
            else defQP[it.key()] = k;
          }
        }
        qpRepo = defQP;
        json qpa = DB::load(uiTXT::QuotingParametersChange);
        if (qpa.size())
          for (json::iterator it = qpa["/0"_json_pointer].begin(); it != qpa["/0"_json_pointer].end(); ++it)
            qpRepo[it.key()] = it.value();
        cleanBool();
        UI::uiSnap(uiTXT::QuotingParametersChange, &onSnap);
        UI::uiHand(uiTXT::QuotingParametersChange, &onHand);
        EV::evUp("QuotingParameters", qpRepo);
        NODE_SET_METHOD(exports, "qpRepo", QP::_qpRepo);
      }
    private:
      static void _qpRepo(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        for (json::iterator it = qpRepo.begin(); it != qpRepo.end(); ++it)
          if (it.value().is_number()) o->Set(FN::v8S(it.key()), Number::New(isolate, it.value()));
          else if (it.value().is_boolean()) o->Set(FN::v8S(it.key()), Boolean::New(isolate, it.value()));
        args.GetReturnValue().Set(o);
      };
      static json onSnap(json z) {
        return { qpRepo };
      };
      static json onHand(json k) {
        if (k["buySize"].get<double>() > 0
          && k["sellSize"].get<double>() > 0
          && k["buySizePercentage"].get<double>() > 0
          && k["sellSizePercentage"].get<double>() > 0
          && k["widthPing"].get<double>() > 0
          && k["widthPong"].get<double>() > 0
          && k["widthPingPercentage"].get<double>() > 0
          && k["widthPongPercentage"].get<double>() > 0
        ) {
          if ((mQuotingMode)k["mode"].get<int>() == mQuotingMode::Depth)
            k["widthPercentage"] = false;
          qpRepo = k;
          cleanBool();
          DB::insert(uiTXT::QuotingParametersChange, k);
          EV::evUp("QuotingParameters", k);
        }
        UI::uiSend(uiTXT::QuotingParametersChange, k);
        return {};
      };
      static void cleanBool() {
        for (vector<string>::iterator it = boolQP.begin(); it != boolQP.end(); ++it)
          if (qpRepo[*it].is_number()) qpRepo[*it] = qpRepo[*it].get<int>() != 0;
      };
  };
}

#endif
