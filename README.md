# termgrid

termcap の制御と Gridインタフェース。
utf-8用。

## 予定

```
struct Cell
{
    char32_t unicode; // 文字
    int cols() const; // 占有するcolumns数。
    int fg; // 色
    int bg; // 背景色
    Flags flags; // 装飾。stdout, underline, bold
};

class Grid
{
    int m_lines;
    int m_cols;
    std::vector<Cell> m_cells;
public:
};
```
