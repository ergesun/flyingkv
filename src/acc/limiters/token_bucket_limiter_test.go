package limiters

import (
    "gopkg.in/check.v1"
    "testing"
    "oss-meta-service/common/config"
    "oss-meta-service/common"
    rwt2 "oss-meta-service/acc/rwt"
    "time"
    "math"
    "context"
)

// Hook up gocheck into the "go test" runner.
func Test(t *testing.T) { check.TestingT(t) }

type TokenBucketLimiterSuite struct {}

var _ = check.Suite(&TokenBucketLimiterSuite{})

func (s *TokenBucketLimiterSuite) TestTokenBucketLimiter(c *check.C) {
    conf := &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:0, MaxResourcesCnt:10000, Speed:50000}
    tb := NewTokenBucketLimiter(conf)
    // Init test
    c.Assert(int64(10000), check.Equals, tb.MaxResourcesCnt)
    c.Assert(int64(0), check.Equals, tb.CurrentResourcesCnt)
    // Update time and get tokens test
    lastUpdate := tb.lastUpdateTs
    now := lastUpdate + 2000
    tb.update(now)
    c.Assert(false, check.Equals, tb.updateAndConsumeTokens(now, 1))
    c.Assert(int64(0), check.Equals, tb.CurrentResourcesCnt)
    now += 17999
    tb.update(now)
    c.Assert(int64(0), check.Equals, tb.CurrentResourcesCnt)
    c.Assert(false, check.Equals, tb.consumeTokens(1))
    now += 20002
    tb.update(now)
    c.Assert(int64(2), check.Equals, tb.CurrentResourcesCnt)
    c.Assert(true, check.Equals, tb.consumeTokens(1))
    c.Assert(false, check.Equals, tb.consumeTokens(2))
    // Put and set tokens test
    tb.putTokens(50)
    c.Assert(int64(51), check.Equals, tb.CurrentResourcesCnt)
    tb.putTokens(110)
    c.Assert(int64(161), check.Equals, tb.CurrentResourcesCnt)
    tb.putTokens(9900)
    c.Assert(tb.MaxResourcesCnt, check.Equals, tb.CurrentResourcesCnt)
    c.Assert(tb.isFull(), check.Equals, true)

    c.Assert(tb.GetName(), check.Equals, conf.Name)
    rwt := &config.RwtConfig{}
    rwt.Weights = make(map[string]int64)
    rwt.Weights[common.ReqGet] = 1
    rwt.Weights[common.ReqPut] = 2
    rwt.Weights[common.ReqDelete] = 1
    rwt.Weights[common.ReqScan] = 20

    tb.rwt = rwt2.NewRwt(rwt)
    tb.consumeTokens(tb.MaxResourcesCnt)

    tb.Skip = true
    c.Assert(tb.GrantWhen(common.RRTypeGet), check.Equals, true)

    tb.Skip = false
    tb.lastUpdateTs = time.Now().UnixNano()
    tb.resNumPerTimeUnit = 0
    tb.timeRemainder = 0
    tb.timeAccumulated = 0

    c.Assert(tb.GrantWhen(common.RRTypeGet), check.Equals, false)
    tb.resNumPerTimeUnit = 1 << 10
    time.Sleep(time.Millisecond)
    c.Assert(tb.GrantWhen(common.RRTypeScan), check.Equals, true)

    c.Assert(tb.GrantWhen(1000), check.Equals, true)
    tb.resNumPerTimeUnit = 0
    tb.CurrentResourcesCnt = 0
    c.Assert(tb.GrantWhen(1000), check.Equals, true)

    c.Assert(tb.GrantWhen(common.RRTypeGet), check.Equals, false)
    tb.GiveBack(common.RRTypeGet)
    c.Assert(tb.GrantWhen(common.RRTypeGet), check.Equals, true)

    conf = &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:0, MaxResourcesCnt:10000, Speed:50}
    tb = NewTokenBucketLimiter(conf)
    c.Assert(tb.CalWaitTime(50), check.Equals, time.Second)
    wt := tb.CalWaitTime(51)
    currentTs := tb.lastUpdateTs + int64(wt)
    c.Check(tb.updateAndConsumeTokens(currentTs, 51), check.Equals, true)

    conf = &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:0, MaxResourcesCnt:10000, Speed:500}
    tb = NewTokenBucketLimiter(conf)
    c.Assert(tb.CalWaitTime(50), check.Equals, time.Second / 10)

    speed := int64(time.Second) + 50
    conf = &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:0, MaxResourcesCnt:math.MaxInt64, Speed:speed}
    tb = NewTokenBucketLimiter(conf)
    c.Assert(tb.CalWaitTime(speed), check.Equals, time.Second)
    wt = tb.CalWaitTime(speed - 50)
    tb.lastUpdateTs = 0
    currentTs = tb.lastUpdateTs + int64(wt - 1)
    c.Check(tb.updateAndConsumeTokens(currentTs, speed - 50), check.Equals, false)
    tb.lastUpdateTs = 0
    tb.CurrentResourcesCnt = 0
    currentTs = tb.lastUpdateTs + int64(wt)
    c.Check(tb.updateAndConsumeTokens(currentTs, speed - 50), check.Equals, true)

    tb.lastUpdateTs = 0
    tb.CurrentResourcesCnt = 0
    currentTs = tb.lastUpdateTs + int64(wt + 1)
    c.Check(tb.updateAndConsumeTokens(currentTs, speed - 50), check.Equals, true)

    rwt = &config.RwtConfig{}
    rwt.Weights = make(map[string]int64)
    rwt.Weights[common.ReqGet] = 50
    rwt.Weights[common.ReqPut] = 2
    rwt.Weights[common.ReqDelete] = 1
    rwt.Weights[common.ReqScan] = 20
    conf = &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:0, MaxResourcesCnt:10000, Speed:50, Rwt:*rwt}

    tb = NewTokenBucketLimiter(conf)
    expectDuration := time.Second
    c.Assert(tb.CalWaitTime(50), check.Equals, expectDuration)

    ctx, _ := context.WithTimeout(context.Background(), expectDuration - 100 * time.Millisecond)
    c.Check(tb.GrantUntil(ctx, common.RRTypeGet), check.Equals, false)

    tb.lastUpdateTs = 0
    tb.CurrentResourcesCnt = 0

    ctx, _ = context.WithTimeout(context.Background(), expectDuration + 10 * time.Millisecond)
    c.Check(tb.GrantUntil(ctx, common.RRTypeGet), check.Equals, true)

    rwt = &config.RwtConfig{}
    rwt.Weights = make(map[string]int64)
    rwt.Weights[common.ReqGet] = 1
    rwt.Weights[common.ReqPut] = 2
    rwt.Weights[common.ReqDelete] = 1
    rwt.Weights[common.ReqScan] = 20000
    conf = &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:1, MaxResourcesCnt:1000, Speed:500, Rwt:*rwt}
    tb = NewTokenBucketLimiter(conf)
    c.Check(tb.CanGrant(common.RRTypeGet), check.Equals, true)
    c.Check(tb.CanGrant(common.RRTypeScan), check.Equals, false)
    c.Check(tb.EstimateGrantWaitTime(common.RRTypeScan), check.Equals, common.MaxDuration)

    rwt = &config.RwtConfig{}
    rwt.Weights = make(map[string]int64)
    rwt.Weights[common.ReqGet] = 1
    rwt.Weights[common.ReqPut] = 2
    rwt.Weights[common.ReqDelete] = 1
    rwt.Weights[common.ReqScan] = 20000
    conf = &config.LimiterConfig{Skip:false, Type:"tb", CurrentResourcesCnt:0, MaxResourcesCnt:1000000, Speed:500, Rwt:*rwt}
    tb = NewTokenBucketLimiter(conf)
    c.Check(tb.CanGrant(common.RRTypeScan), check.Equals, false)
    wt = tb.CalWaitTime(rwt.Weights[common.ReqScan])
    ewt := tb.EstimateGrantWaitTime(common.RRTypeScan)
    c.Check(true, check.Equals, wt >= ewt)
}
