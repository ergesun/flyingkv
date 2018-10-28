package rlc

import (
    . "oss-meta-service/common"
    "oss-meta-service/common/config"
    "oss-meta-service/acc/limiters"
    "context"
    "time"
)

type SimpleResourceLimitControl struct {
    conf        *config.RlcConfig
    limiters     []limiters.Limiter
    limitersCnt  int
}

func NewSimpleResourceLimitControl(conf *config.RlcConfig) *SimpleResourceLimitControl {
    c := &SimpleResourceLimitControl{conf:conf}
    if nil == conf.Limiters || 0 == len(conf.Limiters) {
        return c
    }

    c.limitersCnt = len(conf.Limiters)
    c.limiters = make([]limiters.Limiter, 0, c.limitersCnt)
    for i := range conf.Limiters {
        switch conf.Limiters[i].Type {
        case "token-bucket":
            granter := limiters.NewTokenBucketLimiter(&conf.Limiters[i])
            c.limiters = append(c.limiters, granter)
        case "reference-counter":
            granter := limiters.NewReferenceCountLimiter(&conf.Limiters[i])
            c.limiters = append(c.limiters, granter)
        default:
            panic("not support limiter type.")
        }
    }

    return c
}

func (c *SimpleResourceLimitControl) CanGrant(rt ReqRespType) bool {
    if c.conf.Skip {
        return true
    }

    if 0 == c.limitersCnt {
        return true
    }

    for _, l := range c.limiters {
        if !l.CanGrant(rt) {
            return false
        }
    }

    return true
}

func (c *SimpleResourceLimitControl) EstimateGrantWaitTime(rt ReqRespType) time.Duration {
    if c.conf.Skip {
        return 0
    }

    if 0 == c.limitersCnt {
        return 0
    }

    waitTime := time.Duration(0)
    for _, l := range c.limiters {
        if wt := l.EstimateGrantWaitTime(rt); wt > waitTime {
            waitTime = wt
        }
    }

    return waitTime
}

func (c *SimpleResourceLimitControl) GrantWhen(rt ReqRespType) bool {
    if c.conf.Skip {
        Log.Debugf("req %v passed, skip by ACC.", rt)
        return true
    }

    if 0 == c.limitersCnt {
        Log.Debugf("req %v passed, skip by ACC because it has no limiter.", rt)
        return true
    }

    notOKIdx := -1
    for i := 0; i < c.limitersCnt; i++ {
        if !c.limiters[i].GrantWhen(rt) {
            notOKIdx = i
            break
        }
    }

    if -1 != notOKIdx {
        for i := 0; i < notOKIdx; i++ {
            c.limiters[i].GiveBack(rt)
        }

        Log.Debugf("Request %v Grant rejected by ACC.", rt)
        return false
    }

    Log.Debugf("Request %v Grant passed by ACC.", rt)
    return true
}

func (c *SimpleResourceLimitControl) GrantUntil(ctx context.Context, rt ReqRespType) bool {
    if c.conf.Skip {
        Log.Debugf("req %v passed, skip by ACC.", rt)
        return true
    }

    if 0 == c.limitersCnt {
        Log.Debugf("req %v passed, skip by ACC because it has no limiter.", rt)
        return true
    }

    notOKIdx := -1
    for i := 0; i < c.limitersCnt; i++ {
        if !c.limiters[i].GrantUntil(ctx, rt) {
            notOKIdx = i
            break
        }
    }

    if -1 != notOKIdx {
        for i := 0; i < notOKIdx; i++ {
            c.limiters[i].GiveBack(rt)
        }

        Log.Debugf("Request %v Grant rejected by ACC.", rt)
        return false
    }

    Log.Debugf("Request %v Grant passed by ACC.", rt)
    return true
}

func (c *SimpleResourceLimitControl) GetLimitersForUT() []limiters.Limiter {
    return c.limiters
}