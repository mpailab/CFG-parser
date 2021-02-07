# coding=pymacro
# Вспомогательные правила для макроса match. 
# Они используются при преобразовании узла дерева в питоновский объект функцией syn_expand:
# Если узел соответствует правилу, объявленному с ключевым словом syntax, то он раскрывается описанной функцией
# функция syn_expand применяется к аргументам макроса и других синтаксических расширений, помеченных символом *
syntax(matchcase, pattern: expr, ':', action: suite):
    #print(f'in matchcase 0')
    return (pattern, None, action)
syntax(matchcase, pattern: expr, 'if', cond:test, ':', action: suite):
    #print(f'in matchcase 1')
    return (pattern,cond,action)
syntax(matchcases, x:*matchcase): # здесь к x применяется syn_expand
    #print(f'x = {x}')
    return [x]
syntax(matchcases, xs:*matchcases, x:*matchcase): # здесь к xs и x применяется syn_expand
    if type(xs) is not list:
        xs = [xs]
    #print(f'xs = {xs}, x = {x}')
    xs.append(x)
    return xs

#Максимально упрощённый pattern-matching (просто сравнение на равенство)
def match2cmp(x, pat):
    #print('in match2cmp')
    if pat == `_`: # должна быть встроенная функция, сравнивающая деревья разбора
        return None
    return comparison`${x} == ${pat}`

def make_and(c1, c2):
    if c1 is None: return c2
    if c2 is None: return c1
    return test`($c1) and ($c2)`

defmacro match(stmt, 'match', x:expr, ':', EOL, INDENT, mc:*matchcases, DEDENT):
    #print(f'in match expand, mc = {mc}')
    conds = [(match2cmp(x,p),cond,s) for (p,cond,s) in mc]
    # for i in range(3):
    #     print(f'\nconds[0][{i}] = \n===\n{ast_to_text(parse_context(), conds[0][i])}\n===')
    condexpr = make_and(conds[0][0],conds[0][1])
    if condexpr is None:
        return conds[0][2]
    head = if_stmt_noelse`if $condexpr: ${conds[0][2]}`
    # print(2)
    for (c, cond, s) in conds[1:]:
        condexpr = make_and(c, cond)
        if condexpr is None:
            head = if_stmt`${head} else: ${s}`
            break
        else:
            head = if_stmt_noelse`${head} elif $condexpr: ${s}`
    # print(f'expanded: {ast_to_text(parse_context(), head)}')
    return head

# Тестируем макрос
if __name__ == '__main__':
    for x in range(10):
        match x:
            # _: print(f'{x} -> 0')
            1: print(f'{x} -> a')
            2: print(f'{x} -> b')
            _ if x<5 or x>8: print(f'{x} -> c')
            _: print(f'{x} -> d')
