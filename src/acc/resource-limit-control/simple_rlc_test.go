package rlc

import (
    "gopkg.in/check.v1"
    "testing"
    "oss-meta-service/common/config"
    "oss-meta-service/common"
    "time"
    "oss-meta-service/acc/limiters"
    "context"
)

// Hook up gocheck into the "go test" runner.
func Test(t *testing.T) { check.TestingT(t) }

type SimpleRlcSuite struct {}

var _ = check.Suite(&SimpleRlcSuite{})

func (s *SimpleRlcSuite) TestResourceLimitControl(c *check.C) {
    l1 := config.LimiterConfig{Skip:false, Type:"token-bucket", Name:"tb1", CurrentResourcesCnt:10, MaxResourcesCnt:10, Speed:1}
    rwt1 := config.RwtConfig{}
    rwt1.Weights = make(map[string]int64)
    rwt1.Weights[common.ReqGet] = 1
    rwt1.Weights[common.ReqPut] = 2
    rwt1.Weights[common.ReqDelete] = 1
    rwt1.Weights[common.ReqScan] = 20
    l1.Rwt = rwt1

    l2 := config.LimiterConfig{Skip:false, Type:"token-bucket", Name:"tb2", CurrentResourcesCnt:0, MaxResourcesCnt:10, Speed:1}
    rwt2 := config.RwtConfig{}
    rwt2.Weights = make(map[string]int64)
    rwt2.Weights[common.ReqGet] = 10
    rwt2.Weights[common.ReqPut] = 2
    rwt2.Weights[common.ReqDelete] = 1
    rwt2.Weights[common.ReqScan] = 20
    l2.Rwt = rwt2

    l3 := config.LimiterConfig{Skip:false, Type:"token-bucket", Name:"tb3", CurrentResourcesCnt:10, MaxResourcesCnt:10, Speed:1}
    rwt3 := config.RwtConfig{}
    rwt3.Weights = make(map[string]int64)
    rwt3.Weights[common.ReqGet] = 1
    rwt3.Weights[common.ReqPut] = 2
    rwt3.Weights[common.ReqDelete] = 1
    rwt3.Weights[common.ReqScan] = 20
    l3.Rwt = rwt3

    // test skip
    conf := &config.RlcConfig{Skip:true}
    rlcInst := NewSimpleResourceLimitControl(conf)
    c.Check(rlcInst.GrantWhen(common.RRTypeGet), check.Equals, true)
    c.Check(rlcInst.CanGrant(common.RRTypeGet), check.Equals, true)
    c.Check(rlcInst.EstimateGrantWaitTime(common.RRTypeGet), check.Equals, time.Duration(0))

    // test limiters
    /// test give back
    conf = &config.RlcConfig{Skip:false}
    conf.Limiters = make([]config.LimiterConfig, 0, 2)
    conf.Limiters = append(conf.Limiters, l1, l2)
    rlcInst = NewSimpleResourceLimitControl(conf)

    tb1 := rlcInst.limiters[0].(*limiters.TokenBucketLimiter)
    tb2 := rlcInst.limiters[0].(*limiters.TokenBucketLimiter)
    beforeTokens := tb1.CurrentResourcesCnt
    c.Check(rlcInst.CanGrant(common.RRTypeGet), check.Equals, false)
    c.Check(rlcInst.GrantWhen(common.RRTypeGet), check.Equals, false)
    c.Check(beforeTokens, check.Equals, tb1.CurrentResourcesCnt)
    wt1 := rlcInst.EstimateGrantWaitTime(common.RRTypeGet)
    wt2 := tb2.EstimateGrantWaitTime(common.RRTypeGet)
    c.Check(true, check.Equals, wt1 >= wt2)

    ctx, _ := context.WithTimeout(context.Background(), time.Millisecond)
    beforeTokens = tb1.CurrentResourcesCnt
    c.Check(rlcInst.GrantUntil(ctx, common.RRTypeGet), check.Equals, false)
    c.Check(beforeTokens, check.Equals, tb1.CurrentResourcesCnt)

    /// test multi limiters
    conf = &config.RlcConfig{Skip:false}
    conf.Limiters = make([]config.LimiterConfig, 0, 2)
    conf.Limiters = append(conf.Limiters, l1, l3)
    rlcInst = NewSimpleResourceLimitControl(conf)

    c.Check(rlcInst.CanGrant(common.RRTypeGet), check.Equals, true)
    c.Check(rlcInst.EstimateGrantWaitTime(common.RRTypeGet), check.Equals, time.Duration(0))
    c.Check(rlcInst.GrantWhen(common.RRTypeGet), check.Equals, true)
    ctx, _ = context.WithTimeout(context.Background(), time.Millisecond)
    c.Check(rlcInst.GrantUntil(ctx, common.RRTypeGet), check.Equals, true)
}
