#pragma once
#include <char8/char8.hpp>
#include <functional>

namespace termgrid
{

struct TermPoint
{
    int x;
    int y;
};

struct TermSize
{
    int width;
    int height;
};

// struct TermRect
// {
//     int left;
//     int top;
//     int width;
//     int height;
// };

enum class TermColorTypes : uint8_t
{
    Ansi,
    Color256,
    Color24bit,
};

struct TermColor
{
    uint8_t r; // Color8 or Color256 or Color24bit red
    uint8_t g;
    uint8_t b;
    TermColorTypes type;
};
static_assert(sizeof(TermColor) == 4, "TermColor.sizeof");

/// TermCell にしようと思っていたが可変長になって表現できなかった
///
/// 1コードポイントで1cel: 半角文字
/// 1コードポイントで2cel: 全角文字
/// 1コードポイントで8cel: tab
/// 2コードポイントで2cel: 漢字の異字体セレクタ
/// 2コードポイントで2cel: 合字。濁点、半濁点を合成するやつ
/// 2コードポイントで1cel: 合字。ウムラウトアクサンで合成するやつありそう
/// Nコードポイントで2cel: 絵文字合成
///
/// span<TermCodepoint> で１文字(1Glyph、1 or 2
/// columns)を表現する必要がありそう。 grapheme cluster
///
struct TermCodepoint
{
    // utf8 encoding codepoint
    c8::utf8::codepoint cp;
    /// 合字は最後のコードポイントで >0 にする
    int cols;
    /// standout, underline など Terminal のエフェクト
    int flags;
    TermColor fgcolor;
    TermColor bgcolor;
};

struct TermLine
{
    std::vector<TermCodepoint> codes;

    void clear()
    {
        codes.clear();
    }

    using GetColsFunc = const std::function<int(char32_t)>;

    // return cols
    int push(const GetColsFunc &get_cols, const char8_t *utf8, int size = -1)
    {
        if (size < 0)
        {
            size = 0;
            for (auto p = utf8; *p; ++p)
            {
                ++size;
            }
        }

        int cols = 0;
        for (int i = 0; i < size;)
        {
            auto cp = c8::utf8::codepoint(utf8);
            auto unicode = cp.to_unicode();
            auto c = get_cols(unicode);
            codes.push_back({cp, c});
            cols += c;
            i += cp.codeunit_count();
            utf8 += cp.codeunit_count();
        }
        return cols;
    }

    // return cols
    int push(const GetColsFunc &get_cols, const std::string_view &src)
    {
        return push(get_cols, (const char8_t *)src.data(), src.size());
    }
};

} // namespace termgrid
