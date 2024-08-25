#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

inline bool AreEqual(double d1, double d2, double diff = 0.001) {
    return std::abs(d1 - d2) <= diff;
}

struct DiscountType {
    virtual double DiscountPercent(double price, double quantity) const noexcept = 0;
    virtual ~DiscountType() = default;
};

struct FixedDiscount final : public DiscountType {
    explicit FixedDiscount(double discount) : discount(discount) {}

    double DiscountPercent(double, double) const noexcept override {
        return discount;
    }

  private:
    double discount;
};

struct VolumeDiscount final : public DiscountType {
    explicit VolumeDiscount(double quantity, double discount) noexcept : discount(discount), min_quantity(quantity) {}

    double DiscountPercent(double, double quantity) const noexcept override {
        return quantity >= min_quantity ? discount : 0;
    }

  private:
    double discount;
    double min_quantity;
};

struct PriceDiscount : public DiscountType {
    explicit PriceDiscount(double price, double discount) noexcept : discount(discount), min_total_price(price) {}

    double DiscountPercent(double price, double quantity) const noexcept override {
        return price * quantity >= min_total_price ? discount : 0;
    }

  private:
    double discount;
    double min_total_price;
};

struct AmountDiscount : public DiscountType {
    explicit AmountDiscount(double price, double discount) noexcept : discount(discount), min_total_price(price) {}

    double DiscountPercent(double price, double) const noexcept override {
        return price >= min_total_price ? discount : 0;
    }

  private:
    double discount;
    double min_total_price;
};

struct Customer {
    std::string name;
    DiscountType* discount;
};

enum class ArticleUnit { PIECE, KG, METER, SQMETER, CMETER, LITER };

struct Article {
    int id;
    std::string name;
    double price;
    ArticleUnit unit;
    DiscountType* discount;
};

struct OrderLine {
    Article product;
    int quantity;
    DiscountType* discount;
};

struct Order {
    int id;
    Customer* buyer;
    std::vector<OrderLine> lines;
    DiscountType* discount;
};

struct PriceCalculator {
    virtual double CalculatePrice(const Order& o) = 0;
    virtual ~PriceCalculator() = default;
};

struct CumulativePriceCalculator : public PriceCalculator {
    double CalculatePrice(const Order& o) override {
        double price = 0;
        for (auto ol : o.lines) {
            double line_price = ol.product.price * ol.quantity;

            if (ol.product.discount) {
                line_price *= (1.0 - ol.product.discount->DiscountPercent(ol.product.price, ol.quantity));
            }

            if (ol.discount) {
                line_price *= (1.0 - ol.discount->DiscountPercent(ol.product.price, ol.quantity));
            }

            if (o.buyer && o.buyer->discount) {
                line_price *= (1.0 - o.buyer->discount->DiscountPercent(ol.product.price, ol.quantity));
            }

            price += line_price;
        }

        if (o.discount) {
            price *= (1.0 - o.discount->DiscountPercent(price, 0));
        }

        return price;
    }
};

int main() {
    FixedDiscount d1(0.1);
    VolumeDiscount d2(10, 0.15);
    PriceDiscount d3(100, 0.05);
    AmountDiscount d4(100, 0.05);

    Customer c1{"default", nullptr};
    Customer c2{"john", &d1};
    Customer c3{"joane", &d3};

    Article a1{1, "pen", 5, ArticleUnit::PIECE, nullptr};
    Article a2{2, "expensive pen", 15, ArticleUnit::PIECE, &d1};
    Article a3{3, "scissors", 10, ArticleUnit::PIECE, &d2};

    CumulativePriceCalculator calc;

    Order o1 {101, &c1, {{a1, 1, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o1), 5));

    Order o2{102, &c2, {{a1, 1, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o2), 4.5));

    Order o3 {103, &c1, {{a2, 1, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o3), 13.5));

    Order o4 {104, &c2, {{a2, 1, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o4), 12.15));

    Order o5 {105, &c1, {{a3, 1, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o5), 10));

    Order o6 {106, &c1, {{a3, 15, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o6), 127.5));

    Order o7 {107, &c3, {{a3, 15, nullptr}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o7), 121.125));

    Order o8 {108, &c3, {{a2, 20, &d1}}, nullptr};
    assert(AreEqual(calc.CalculatePrice(o8), 230.85));

    Order o9 {109, &c3, {{a2, 20, &d1}}, &d4};
    assert(AreEqual(calc.CalculatePrice(o9), 219.3075));
}
